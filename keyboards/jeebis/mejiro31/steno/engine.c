// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include "chord.h"
#include "../../../../../Mejiro_Engine/native/mejiro.h"
#include "send_string.h"

#include <string.h>

extern void mejiro_qmk_init(void);
extern void mejiro_qmk_reset(void);
extern int32_t mejiro_qmk_process(const uint8_t *, size_t, mejiro_event *);
extern int32_t mejiro_qmk_ack(uint64_t, uint32_t);
extern int32_t mejiro_qmk_preview(const uint8_t *, size_t, mejiro_translation *);
extern int32_t mejiro_qmk_preview_kana(const uint8_t *, size_t, mejiro_translation *);
extern bool mejiro_qmk_is_recording(size_t);
extern int32_t mejiro_qmk_record_external(const mejiro_hid_event *, size_t);
extern int32_t mejiro_qmk_replayed_external(mejiro_recorded_event *, size_t);
extern void mejiro_qmk_abort_recordings(void);

static mejiro_hid_event pending_keyboard[MEJIRO_EXTERNAL_CAPACITY];
static mejiro_recorded_event replay_events[MEJIRO_EXTERNAL_CAPACITY];
static size_t pending_count;
static bool recording_failed, replaying;
static uint8_t aborted_slots;

static void abort_capture(void) {
    for (size_t i = 0; i < 7; ++i)
        if (mejiro_qmk_is_recording(i)) aborted_slots |= (uint8_t)(1U << i);
    mejiro_qmk_abort_recordings();
    pending_count = 0;
    recording_failed = true;
}
void mejiro_engine_record(mejiro_hid_event event) {
    if (replaying) return;
    bool active = false;
    for (size_t i = 0; i < 7; ++i) active |= mejiro_qmk_is_recording(i);
    if (!active) return;
    if (pending_count == MEJIRO_EXTERNAL_CAPACITY) {
        abort_capture();
        return;
    }
    pending_keyboard[pending_count++] = event;
}
bool mejiro_engine_flush_keyboard(void) {
    if (!pending_count) return true;
    if (mejiro_qmk_record_external(pending_keyboard, pending_count) != 0) {
        abort_capture();
        return false;
    }
    pending_count = 0;
    return true;
}
bool mejiro_engine_recording_failed(void) { return recording_failed; }

#define MEJIRO_HISTORY 20
#define MEJIRO_MACROS 7

static uint32_t undo_units[MEJIRO_HISTORY];
static uint8_t undo_count;
static uint32_t macro_units[MEJIRO_MACROS];

typedef struct {
    const char *glyph;
    const char *keys;
} ime_glyph_t;

// Japanese IME's conventional punctuation shortcuts. Grammar and word
// conversion remain in Rust; these are only host key sequences.
static const ime_glyph_t ime_glyphs[] = {
    {"、", ","}, {"。", "."}, {"・", "z/"}, {"※", "z*"},
    {"「", "["}, {"」", "]"}, {"『", "z["}, {"』", "z]"},
    {"（", "("}, {"）", ")"}, {"【", "z("}, {"】", "z)"},
    {"〈", "<"}, {"〉", ">"}, {"《", "z<"}, {"》", "z>"},
    {"‖", "z|"}, {"…", "z."}, {"←", "zh"}, {"↓", "zj"},
    {"↑", "zk"}, {"→", "zl"}, {"～", "~"}, {"＊", "*"},
    {"？", "?"}, {"！", "!"}, {"　", " "},
};

static uint16_t ascii_key(char c) {
    switch (c) {
        case '"': return KC_DQUO;
        case '\'': return KC_QUOT;
        case '|': return KC_PIPE;
        case ':': return KC_COLN;
        case '/': return KC_SLSH;
        case '*': return KC_ASTR;
        case '~': return KC_TILD;
        case '^': return KC_CIRC;
        case '(': return KC_LPRN;
        case ')': return KC_RPRN;
        case '[': return KC_LBRC;
        case ']': return KC_RBRC;
        case '{': return KC_LCBR;
        case '}': return KC_RCBR;
        case '<': return KC_LABK;
        case '>': return KC_RABK;
        case '.': return KC_DOT;
        case ',': return KC_COMM;
        case '?': return KC_QUES;
        case '!': return KC_EXLM;
        case '@': return KC_AT;
        case '#': return KC_HASH;
        case ' ': return KC_SPC;
        default: return KC_NO;
    }
}

static void send_ascii(char c) {
    uint16_t key = ascii_key(c);
    if (mejiro_steno_jis_host() && key != KC_NO) {
        mejiro_steno_tap(key);
    } else {
        send_char(c);
    }
}

static bool next_text(const uint8_t *text, size_t length, size_t *position, const char **keys) {
    uint8_t first = text[*position];
    if (first > 0 && first < 128) {
        *keys = NULL;
        ++*position;
        return true;
    }
    for (size_t i = 0; i < sizeof(ime_glyphs) / sizeof(ime_glyphs[0]); ++i) {
        size_t bytes = strlen(ime_glyphs[i].glyph);
        if (bytes <= length - *position && memcmp(text + *position, ime_glyphs[i].glyph, bytes) == 0) {
            *keys = ime_glyphs[i].keys;
            *position += bytes;
            return true;
        }
    }
    return false;
}

__attribute__((weak)) bool mejiro_steno_select_alternate_system(void) {
    return false;
}

static bool supported_key(uint8_t key) {
    return key >= MEJIRO_RETURN && key <= MEJIRO_IME_OFF;
}

static bool can_send(const mejiro_translation *translation) {
    size_t offset = 0;
    const char *keys;
    while (offset < translation->text_len) {
        if (!next_text(translation->text, translation->text_len, &offset, &keys)) return false;
    }
    if (translation->operation_count > 8) return false;
    size_t previous = 0;
    for (uint8_t i = 0; i < translation->operation_count; ++i) {
        mejiro_operation op = translation->operations[i];
        if (!supported_key(op.key) || op.byte_offset < previous || op.byte_offset > translation->text_len) return false;
        previous = op.byte_offset;
    }
    return true;
}

static void send_text(const uint8_t *text, size_t length) {
    size_t offset = 0;
    while (offset < length) {
        size_t start = offset;
        const char *keys;
        if (!next_text(text, length, &offset, &keys)) return;
        if (keys) {
            while (*keys) send_ascii(*keys++);
        } else {
            send_ascii((char)text[start]);
        }
    }
}

static void send_operation(uint8_t key) {
    if (key == MEJIRO_SELECT_ALTERNATE_SYSTEM) {
        (void)mejiro_steno_select_alternate_system();
        return;
    }
    static const uint16_t qmk_keys[] = {
        [MEJIRO_RETURN] = KC_ENT, [MEJIRO_SPACE] = KC_SPC, [MEJIRO_TAB] = KC_TAB,
        [MEJIRO_BACKSPACE] = KC_BSPC, [MEJIRO_DELETE] = KC_DEL,
        [MEJIRO_LEFT] = KC_LEFT, [MEJIRO_DOWN] = KC_DOWN,
        [MEJIRO_RIGHT] = KC_RIGHT, [MEJIRO_UP] = KC_UP,
        [MEJIRO_HOME] = KC_HOME, [MEJIRO_END] = KC_END,
        [MEJIRO_SHIFT_LEFT] = LSFT(KC_LEFT), [MEJIRO_SHIFT_DOWN] = LSFT(KC_DOWN),
        [MEJIRO_SHIFT_RIGHT] = LSFT(KC_RIGHT), [MEJIRO_SHIFT_UP] = LSFT(KC_UP),
        [MEJIRO_SHIFT_HOME] = LSFT(KC_HOME), [MEJIRO_SHIFT_END] = LSFT(KC_END),
        [MEJIRO_SHIFT_RETURN] = LSFT(KC_ENT), [MEJIRO_CTRL_RETURN] = LCTL(KC_ENT),
        [MEJIRO_F13] = KC_F13, [MEJIRO_F14] = KC_F14,
        [MEJIRO_SHIFT_0] = LSFT(KC_0),
        [MEJIRO_IME_ON] = KC_LNG1, [MEJIRO_IME_OFF] = KC_LNG2,
    };
    if (supported_key(key)) mejiro_steno_tap(qmk_keys[key]);
}

static bool valid_replay(const mejiro_translation *translation, size_t count) {
    uint16_t previous_byte = 0;
    uint8_t previous_op = 0;
    for (size_t i = 0; i < count; ++i) {
        mejiro_recorded_event e = replay_events[i];
        if (e.byte_offset > translation->text_len || e.operation_index > translation->operation_count || e.byte_offset < previous_byte || e.operation_index < previous_op) return false;
        if (e.operation_index && e.byte_offset < translation->operations[e.operation_index - 1].byte_offset) return false;
        if (e.operation_index < translation->operation_count && e.byte_offset > translation->operations[e.operation_index].byte_offset) return false;
        if (e.event.kind > 2 || (e.event.page != 1 && e.event.page != 7 && e.event.page != 12)) return false;
        previous_byte = e.byte_offset;
        previous_op = e.operation_index;
    }
    return true;
}
static void send_output(const mejiro_translation *translation, size_t external_count) {
    size_t from = 0;
    size_t external = 0;
    replaying = true;
    if (external_count) mejiro_keyboard_replay_begin();
    for (uint8_t i = 0; i <= translation->operation_count; ++i) {
        while (external < external_count && replay_events[external].operation_index == i) {
            mejiro_recorded_event e = replay_events[external++];
            send_text(translation->text + from, e.byte_offset - from);
            from = e.byte_offset;
            mejiro_keyboard_replay_event(&e.event);
        }
        if (i < translation->operation_count) {
            mejiro_operation op = translation->operations[i];
            send_text(translation->text + from, op.byte_offset - from);
            send_operation(op.key);
            from = op.byte_offset;
        }
    }
    send_text(translation->text + from, translation->text_len - from);
    if (external_count) mejiro_keyboard_replay_end();
    replaying = false;
}

static uint32_t kana_units(const mejiro_translation *translation) {
    uint32_t units = 0;
    size_t offset = 0;
    while (offset < translation->text_len) {
        if ((translation->text[offset] & 0xc0) != 0x80) ++units;
        ++offset;
    }
    for (uint8_t i = 0; i < translation->operation_count; ++i) {
        switch (translation->operations[i].key) {
            case MEJIRO_RETURN:
            case MEJIRO_SPACE:
            case MEJIRO_TAB: ++units; break;
            default: break;
        }
    }
    return units;
}

static void remember_units(uint32_t units) {
    if (undo_count == MEJIRO_HISTORY) {
        memmove(undo_units, undo_units + 1, (MEJIRO_HISTORY - 1) * sizeof(*undo_units));
        --undo_count;
    }
    undo_units[undo_count++] = units;
}

void mejiro_engine_init(void) {
    mejiro_qmk_init();
    undo_count = 0;
    memset(macro_units, 0, sizeof(macro_units));
    pending_count = 0;
    recording_failed = replaying = false;
    aborted_slots = 0;
}

void mejiro_engine_reset(void) {
    mejiro_qmk_reset();
    undo_count = 0;
}

void mejiro_engine_process(const char *stroke, uint8_t length) {
    if (!length) return;
    // Batch import is record-only; the host already received these key events.
    mejiro_engine_flush_keyboard();
    mejiro_translation preview;
    if (mejiro_qmk_preview((const uint8_t *)stroke, length, &preview) != 0) return;
    // A slot toggle intended to stop a failed capture must not accidentally
    // start a fresh recording after the automatic abort cleared that slot.
    if (preview.request == 4 && preview.argument < MEJIRO_MACROS && (aborted_slots & (1U << preview.argument))) {
        aborted_slots &= (uint8_t)~(1U << preview.argument);
        return;
    }
    if (preview.request == 5) aborted_slots = 0;
    if (preview.request == 0 && !can_send(&preview)) return;

    mejiro_translation kana;
    uint32_t units = 0;
    if (preview.request == 0 && mejiro_qmk_preview_kana((const uint8_t *)stroke, length, &kana) == 0) {
        units = kana_units(&kana);
    } else if (preview.request == 2 && undo_count) {
        units = undo_units[undo_count - 1];
    } else if (preview.request == 6 && preview.argument < MEJIRO_MACROS) {
        units = macro_units[preview.argument];
    }

    mejiro_event event;
    int32_t status = mejiro_qmk_process((const uint8_t *)stroke, length, &event);
    if (status == -2) {
        // The core rejects an overfull recorder atomically. Drop the failed
        // capture, then retry so an ordinary steno input still reaches the host.
        bool active = false;
        for (size_t i = 0; i < MEJIRO_MACROS; ++i) active |= mejiro_qmk_is_recording(i);
        if (active) {
            abort_capture();
            status = mejiro_qmk_process((const uint8_t *)stroke, length, &event);
        }
    }
    if (status != 0) return;
    if (event.kind == 2) {
        if (preview.request == 4 && preview.argument < MEJIRO_MACROS && mejiro_qmk_is_recording(preview.argument)) {
            macro_units[preview.argument] = 0;
            recording_failed = false;
            aborted_slots = 0;
        }
        return;
    }
    if (event.kind == 1) {
        for (uint32_t i = 0; i < event.erase_units; ++i) mejiro_steno_tap(KC_BSPC);
        if (undo_count) --undo_count;
        return;
    }
    if (!can_send(&event.translation)) {
        mejiro_engine_reset();
        return;
    }
    int32_t external_count = mejiro_qmk_replayed_external(replay_events, MEJIRO_EXTERNAL_CAPACITY);
    if (external_count < 0 || !valid_replay(&event.translation, (size_t)external_count)) {
        abort_capture();
        mejiro_engine_reset();
        return;
    }
    send_output(&event.translation, (size_t)external_count);
    if (event.has_ticket) {
        if (event.translation.text_len || event.translation.operation_count) {
            if (mejiro_qmk_ack(event.ticket, units) != 0) {
                mejiro_engine_reset();
                return;
            }
        }
        remember_units(units);
    } else if (event.translation.text_len || event.translation.operation_count || external_count) {
        undo_count = 0;
    }
    for (uint8_t i = 0; i < MEJIRO_MACROS; ++i) {
        if (mejiro_qmk_is_recording(i)) macro_units[i] += units;
    }
}
