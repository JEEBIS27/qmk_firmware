// Shared implementation included by each declarative keymap.
// SPDX-License-Identifier: GPL-2.0-or-later
#include QMK_KEYBOARD_H
#include "mejiro_input.h"
#include "steno/chord.h"
#include "steno/engine.h"
#include "layouts/catalog.h"
#include "keymap_introspection.h"
#include "process_steno.h"
#include "os_detection.h"
#include "eeconfig.h"
#include <string.h>

// Tap layers 0..5; matching Hold maps are stored at 6..11.
enum { MJI_QWERTY, MJI_SHIFTED, MJI_NUMBER, MJI_NUMBER_SHIFTED, MJI_FUNCTION, MJI_STENO, MJI_HOLD_OFFSET };
enum { MJI_00 = SAFE_RANGE, MJI_000, MJI_MODE, MJI_JIS, MJI_ALT, MJI_MEJIRO, MJI_STICKY };

#include "defaults.h"

// Physical coordinates, independent of key assignments or language/layout.
// logical is a free matrix-storage cell, not another wired switch.
static const mji_pair_t physical_pairs[] = {
    {1, 13, 24, 0}, {2, 14, 25, 0}, {3, 15, 26, 0}, {4, 16, 27, 0}, {5, 17, 31, 0}, {6, 18, 32, 0}, {7, 19, 33, 0}, {8, 20, 34, 0}, {9, 21, 35, 0}, {10, 22, 36, 0}, {11, 23, 37, 0}, {4, 5, 38, 1}, {16, 17, 39, 1}, {6, 7, 44, 1}, {18, 19, 45, 1}, {42, 43, 46, 1},
};

static mji_engine_t input;
static bool         mejiro_mode = MJI_DEFAULT_MEJIRO, steno_mode, ready, mac;
static uint8_t      host_mods;
static uint8_t      keyboard_language;
static uint16_t     held_codes[MJI_TOKEN_COUNT];
static uint8_t      refs[256];
static uint16_t     steno_keys[MJI_PHYSICAL_COUNT];
static uint64_t     steno_held;
static uint64_t     gemini_held;
static bool         shortcut_active, discard_steno_send;
static bool         replay_down[256];
static uint8_t      replay_saved_mods;

static void capture_key(uint8_t code, bool pressed) {
    if (!code) return;
    mejiro_hid_event event = {.usage = code, .page = 7, .mods = get_mods(), .kind = pressed ? 1 : 2};
    if (IS_CONSUMER_KEYCODE(code)) {
        event.page = 12;
        event.usage = KEYCODE2CONSUMER(code);
    } else if (IS_SYSTEM_KEYCODE(code)) {
        event.page = 1;
        event.usage = KEYCODE2SYSTEM(code);
    }
    mejiro_engine_record(event);
}
static void register_output(uint8_t code) {
    capture_key(code, true);
    register_code(code);
}
static void unregister_output(uint8_t code) {
    capture_key(code, false);
    unregister_code(code);
}
static void tap_output(uint8_t code) {
    capture_key(code, true);
    tap_code16(code);
    capture_key(code, false);
}
void mejiro_keyboard_replay_begin(void) {
    replay_saved_mods = get_mods();
    memset(replay_down, 0, sizeof(replay_down));
    clear_keyboard();
}
void mejiro_keyboard_replay_event(const mejiro_hid_event *event) {
    set_mods(event->mods);
    set_weak_mods(0);
    clear_oneshot_mods();
    send_keyboard_report();
    if (!event->kind) return;
    if (event->page == 7 && event->usage < 256) {
        if (event->kind == 1) register_code(event->usage);
        else unregister_code(event->usage);
        replay_down[event->usage] = event->kind == 1;
    } else if (event->page == 12) host_consumer_send(event->kind == 1 ? event->usage : 0);
    else if (event->page == 1) host_system_send(event->kind == 1 ? event->usage : 0);
}
void mejiro_keyboard_replay_end(void) {
    for (unsigned code = 1; code < 256; ++code)
        if (replay_down[code]) unregister_code(code);
    host_consumer_send(0);
    host_system_send(0);
    clear_keyboard();
    set_mods(replay_saved_mods);
    for (unsigned code = 1; code < 256; ++code)
        if (refs[code]) register_code(code);
    send_keyboard_report();
}

bool send_steno_chord_user(steno_mode_t mode, uint8_t chord[]) {
    (void)mode;
    (void)chord;
    return !discard_steno_send;
}
static bool shortcut_held(void) {
    if (!steno_mode) return false;
    for (uint8_t i = 0; i < MJI_TOKEN_COUNT; ++i) {
        mji_token_t *t = &input.tokens[i];
        if ((t->state == MJI_ACTIVE || t->state == MJI_PREPARED) && (t->binding.hold_mods & ~MOD_MASK_SHIFT)) return true;
    }
    return false;
}
static void cancel_steno_input(void) {
    // Drain QMK's pressed-key counter without transmitting a partial chord.
    discard_steno_send = true;
    for (uint8_t p = 0; p < MJI_PHYSICAL_COUNT; ++p) {
        if (!(gemini_held & (UINT64_C(1) << p))) continue;
        keyrecord_t release = {.event = {.key = {.row = p / MATRIX_COLS, .col = p % MATRIX_COLS}, .pressed = false}};
        process_steno(steno_keys[p], &release);
    }
    discard_steno_send = false;
    input.ignored |= steno_held;
    gemini_held = steno_held = 0;
    memset(steno_keys, 0, sizeof(steno_keys));
    mejiro_reset_state();
}
static void sync_shortcut(void) {
    bool active = shortcut_held();
    if (active && !shortcut_active) cancel_steno_input();
    if (!active && shortcut_active) mejiro_engine_flush_keyboard();
    shortcut_active = active;
}

// Versioned user word, independent of VIA's editable keymap storage.
#define MJI_SETTINGS_MAGIC UINT32_C(0x4d310100)
static void save_settings(void) {
    uint32_t word = MJI_SETTINGS_MAGIC | input.layout.jis_host | ((uint32_t)input.layout.alt_enabled << 1) | ((uint32_t)mejiro_mode << 2);
    if (eeconfig_read_user() != word) eeconfig_update_user(word);
}
void eeconfig_init_user(void) {
    eeconfig_update_user(MJI_SETTINGS_MAGIC | MJI_DEFAULT_JIS | UINT32_C(2) | ((uint32_t)MJI_DEFAULT_MEJIRO << 2));
}

static uint8_t expand_mods(uint8_t mods) {
    return mods & 0x10 ? (mods & 0x0f) << 4 : mods;
}
static uint8_t platform_mods(uint8_t mods) {
    if (!mac) return mods;
    return (mods & ~(MOD_MASK_CTRL | MOD_MASK_GUI)) | ((mods & MOD_MASK_CTRL) << 3) | ((mods & MOD_MASK_GUI) >> 3);
}
static void report_mods(uint8_t mods) {
    uint8_t native = platform_mods(mods);
    if (native != get_mods()) mejiro_engine_record((mejiro_hid_event){.page = 7, .mods = native});
    set_mods(native);
    set_weak_mods(0);
    clear_oneshot_mods();
    send_keyboard_report();
}
static void mods_callback(void *user, uint8_t mods) {
    (void)user;
    host_mods = mods;
    report_mods(mods);
}
static void clear_callback(void *user) {
    (void)user;
    // These releases really go to the host during a mode boundary. Complete
    // their recorded stream even after steno_mode has already been toggled.
    for (unsigned code = 1; code < 256; ++code)
        if (refs[code]) capture_key(code, false);
    if (get_mods()) mejiro_engine_record((mejiro_hid_event){.page = 7});
    cancel_steno_input();
    shortcut_active = false;
    memset(held_codes, 0, sizeof(held_codes));
    memset(refs, 0, sizeof(refs));
    memset(steno_keys, 0, sizeof(steno_keys));
    steno_held = 0;
    host_mods  = 0;
    clear_keyboard();
    layer_clear();
    steno_set_mode(STENO_MODE_GEMINI);
}
static void switch_context(void) {
    mji_reset(&input, steno_mode ? MJI_STENO : MJI_QWERTY);
    default_layer_set((layer_state_t)1 << input.base_layer);
}

// Platform codec only. Layout policy and symbol translation live in the engine.
static bool decode_callback(void *user, uint16_t code, mji_key_t *key) {
    (void)user;
    if (IS_QK_MODS(code)) {
        *key = (mji_key_t){QK_MODS_GET_BASIC_KEYCODE(code), expand_mods(QK_MODS_GET_MODS(code))};
        return true;
    }
    if (code <= QK_BASIC_MAX) {
        *key = (mji_key_t){code, 0};
        return true;
    }
    return false;
}
static bool select_layout(const mji_layout_config_t *config) {
    return mji_set_layout(&input, config);
}
static mji_stroke_t encode_callback(void *user, mji_stroke_t stroke, uint8_t mods) {
    (void)user;
    if (mac && (stroke.code == KC_HOME || stroke.code == KC_END)) {
        uint8_t effective = (mods & ~stroke.clear_mods) | stroke.mods;
        bool document = platform_mods(effective) & MOD_MASK_GUI;
        stroke.code = stroke.code == KC_HOME ? (document ? KC_UP : KC_LEFT) : (document ? KC_DOWN : KC_RIGHT);
        // Logical Ctrl becomes Command at the platform boundary. Preserve
        // Shift/Alt; suppress host Ctrl for the document-boundary variant.
        stroke.mods |= MOD_LCTL;
        if (document) {
            stroke.mods &= ~MOD_MASK_GUI;
            stroke.clear_mods |= MOD_MASK_GUI;
        }
    }
    return stroke;
}

// Steno is a separate input mode; its explicit taps share host-layout encoding
// without passing through keyboard compression or alternative-letter mappings.
bool mejiro_steno_jis_host(void) {
    return input.layout.jis_host;
}
void mejiro_steno_tap(uint16_t code) {
    mji_key_t key;
    if (!decode_callback(NULL, code, &key)) {
        tap_code16(code);
        return;
    }
    mji_layout_config_t config = input.layout;
    config.alt_enabled         = false;
    mji_stroke_t stroke        = mji_layout_resolve(&config, key, MJI_STENO, 0);
    uint8_t      saved         = get_mods();
    // Convert the saved platform mods back to logical mods before encoding.
    stroke.mods = platform_mods(stroke.mods);
    stroke.clear_mods = platform_mods(stroke.clear_mods);
    stroke = encode_callback(NULL, stroke, platform_mods(saved));
    set_mods(platform_mods((platform_mods(saved) & ~stroke.clear_mods) | stroke.mods));
    send_keyboard_report();
    tap_code16(stroke.code);
    set_mods(saved);
    send_keyboard_report();
}
static mji_binding_t binding(uint16_t tap, uint16_t hold) {
    mji_binding_t b = {.tap = tap, .hold_layer = MJI_NO_LAYER};
    // Native modifier/layer actions use the same engine Hold representation.
    if (!hold && (IS_MODIFIER_KEYCODE(tap) || IS_QK_MOMENTARY(tap))) {
        hold  = tap;
        b.tap = KC_NO;
    }
    if (IS_MODIFIER_KEYCODE(hold))
        b.hold_mods = MOD_BIT(hold);
    else if (IS_QK_MOMENTARY(hold))
        b.hold_layer = QK_MOMENTARY_GET_LAYER(hold);
    else if (hold != KC_TRNS)
        b.hold_code = hold;
    if (b.tap == MJI_STICKY || (IS_QK_ONE_SHOT_MOD(b.tap) && (expand_mods(QK_ONE_SHOT_MOD_GET_MODS(b.tap)) & MOD_MASK_SHIFT))) b.tap = MJI_STICKY_SHIFT;
    if (b.tap == KC_TRNS) b.tap = KC_NO;
    return b;
}
static mji_binding_t lookup_callback(void *user, uint16_t logical, const mji_context_t *c) {
    (void)user;
    for (uint8_t i = 0; i < c->layer_count; i++) {
        uint8_t layer = c->layers[i];
        if (layer >= MJI_HOLD_OFFSET) continue;
        if (steno_mode && (c->mods & ~MOD_MASK_SHIFT) && layer == MJI_STENO) layer = MJI_QWERTY;
        if (c->mods & MOD_MASK_SHIFT) {
            if (layer == MJI_QWERTY)
                layer = MJI_SHIFTED;
            else if (layer == MJI_NUMBER)
                layer = MJI_NUMBER_SHIFTED;
        }
        uint8_t  row = logical / MATRIX_COLS, col = logical % MATRIX_COLS;
        uint16_t tap  = keycode_at_keymap_location(layer, row, col);
        uint16_t hold = keycode_at_keymap_location(layer + MJI_HOLD_OFFSET, row, col);
        if (tap == KC_TRNS && hold == KC_TRNS) continue;
        mji_binding_t b = binding(tap, hold);
        b.layer         = layer;
        return b;
    }
    return (mji_binding_t){.hold_layer = MJI_NO_LAYER};
}
static bool custom_action(uint16_t code, bool pressed) {
    switch (code) {
        case MJI_MODE:
            if (pressed) {
                mji_layout_config_t config = input.layout;
                if (!steno_mode) keyboard_language = config.language;
                steno_mode = !steno_mode;
                bool ime_on_entry = !MJI_EN_PROFILE && steno_mode && mejiro_mode;
                if (ime_on_entry) config.language = MJI_JAPANESE;
                if (!steno_mode) config.language = keyboard_language;
                switch_context();
                if (config.language != input.layout.language) select_layout(&config);
                // Send after releasing the previous context's keys and mods.
                if (ime_on_entry || !steno_mode) {
                    uint8_t language = config.language == MJI_JAPANESE ? KC_LNG1 : KC_LNG2;
                    capture_key(language, true);
                    tap_code16(language);
                    capture_key(language, false);
                }
                if (steno_mode) mejiro_engine_flush_keyboard();
            }
            return true;
        case MJI_JIS:
            if (pressed) {
                mji_layout_config_t config = input.layout;
                config.jis_host            = !config.jis_host;
                select_layout(&config);
                save_settings();
            }
            return true;
        case MJI_ALT:
            if (pressed) {
                mji_layout_config_t config = input.layout;
                config.alt_enabled         = !config.alt_enabled;
                select_layout(&config);
                save_settings();
            }
            return true;
        case MJI_MEJIRO:
            if (pressed) {
                mejiro_mode = !mejiro_mode;
                switch_context();
                save_settings();
            }
            return true;
        case KC_LNG1:
        case KC_LNG2:
            if (pressed) {
                mji_layout_config_t config = input.layout;
                config.language            = code == KC_LNG1 ? MJI_JAPANESE : MJI_ENGLISH;
                select_layout(&config);
                tap_output(code);
            }
            return true;
        case QK_BOOTLOADER:
            if (pressed) {
                switch_context();
                reset_keyboard();
            }
            return true;
        default:
            return false;
    }
}
static void key_callback(void *user, uint8_t source, uint16_t code, bool pressed, uint8_t mods) {
    (void)user;
    if (custom_action(code, pressed)) return;
    // Unsupported QMK feature encodings must not truncate into unrelated HID
    // keys. The portable engine can represent them; add an explicit adapter
    // action before exposing such a feature in the editor.
    if (code > QK_BASIC_MAX && code != MJI_00 && code != MJI_000) return;
    if (pressed) {
        bool zero_action = code == MJI_00 || code == MJI_000;
        // Emit the extra zero(s), then retain an ordinary 0 for OS repeat.
        if (zero_action) {
            // Pulse without invalidating another logical owner's held 0.
            if (refs[KC_0]) unregister_output(KC_0);
            report_mods(mods);
            tap_output(KC_0);
            if (code == MJI_000) tap_output(KC_0);
            code = KC_0;
        }
        held_codes[source] = code;
        report_mods(mods);
        uint8_t basic = code;
        if (!refs[basic]++ || zero_action) register_output(basic);
    } else {
        code = held_codes[source];
        if (!code) return;
        uint8_t basic = code;
        if (refs[basic] && !--refs[basic]) unregister_output(basic);
        held_codes[source] = 0;
    }
    report_mods(mods);
}

void keyboard_post_init_user(void) {
    mejiro_engine_init();
    mac = detected_host_os() == OS_MACOS || detected_host_os() == OS_IOS;
#ifdef NKRO_ENABLE
    keymap_config.nkro = true;
#endif
    uint32_t settings = eeconfig_read_user();
    if ((settings & ~UINT32_C(7)) != MJI_SETTINGS_MAGIC) {
        eeconfig_init_user();
        settings = eeconfig_read_user();
    }
    mejiro_mode = (settings & 4) != 0;
    mji_callbacks_t cb = {.lookup = lookup_callback, .key = key_callback, .mods = mods_callback, .clear = clear_callback, .decode = decode_callback, .encode = encode_callback};
    ready              = mji_init(&input, physical_pairs, sizeof(physical_pairs) / sizeof(*physical_pairs), (mji_timing_t){MJI_COMBO_MS, MJI_CONTINUATION_MS, MJI_LONG_HOLD_MS}, MJI_QWERTY, cb, NULL);
    if (ready) {
        mji_layout_config_t config = {.maps = {MJI_LAYOUT_MAP(MJI_ENGLISH_LAYOUT), MJI_LAYOUT_MAP(MJI_JAPANESE_LAYOUT)}, .alt_layers = (UINT32_C(1) << MJI_QWERTY) | (UINT32_C(1) << MJI_SHIFTED), .language = MJI_ENGLISH, .alt_enabled = true, .jis_host = MJI_DEFAULT_JIS};
        config.jis_host = (settings & 1) != 0;
        config.alt_enabled = (settings & 2) != 0;
        ready                      = select_layout(&config);
    }
    default_layer_set((layer_state_t)1 << MJI_QWERTY);
    steno_set_mode(STENO_MODE_GEMINI);
}

bool pre_process_record_user(uint16_t keycode, keyrecord_t *record) {
    (void)keycode;
    if (!ready || record->event.key.row >= MATRIX_ROWS || record->event.key.col >= MATRIX_COLS) return false;
    uint8_t  p       = record->event.key.row * MATRIX_COLS + record->event.key.col;
    uint64_t mask    = UINT64_C(1) << p;
    bool     pressed = record->event.pressed;
    sync_shortcut();
    // Steno keys bypass keyboard compression, but share the mode-reset barrier.
    bool steno_layer = steno_mode && !shortcut_active;
    for (uint8_t i = 0; i < MJI_TOKEN_COUNT; i++) {
        if ((input.tokens[i].state == MJI_ACTIVE || input.tokens[i].state == MJI_PREPARED) && input.tokens[i].binding.hold_layer != MJI_NO_LAYER) steno_layer = false;
    }
    // A key routed through the keyboard engine keeps that release path even
    // if its modifier was released before it.
    if (steno_mode && !(input.ignored & mask) && (pressed || !input.owner[p])) {
        uint16_t kc = pressed ? (steno_layer ? keycode_at_keymap_location(MJI_STENO, p / MATRIX_COLS, p % MATRIX_COLS) : KC_NO) : steno_keys[p];
        if (is_stn_key(kc)) {
            if (pressed) {
                steno_keys[p] = kc;
                steno_held |= mask;
                input.physical |= mask;
            } else {
                steno_held &= ~mask;
                gemini_held &= ~mask;
                input.physical &= ~mask;
                steno_keys[p] = 0;
            }
            if (mejiro_mode) {
                if (pressed)
                    mejiro_on_press(kc);
                else {
                    mejiro_on_release(kc);
                }
            } else {
                if (pressed) gemini_held |= mask;
                process_steno(kc, record);
            }
            return false;
        }
    }
    if (steno_mode && mejiro_mode && pressed) mejiro_engine_reset();
    mji_event(&input, p, pressed, timer_read32());
    sync_shortcut();
    return false; // Raw keyboard events are fully consumed by this engine.
}
void matrix_scan_user(void) {
    if (!ready) return;
    mji_tick(&input, timer_read32());
    sync_shortcut();
    // OS detection may finish after startup; don't change mapping mid-hold.
    if (!input.physical && !input.event_count) mac = detected_host_os() == OS_MACOS || detected_host_os() == OS_IOS;
}
