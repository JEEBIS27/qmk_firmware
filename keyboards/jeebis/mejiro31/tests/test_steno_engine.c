// Host-side test of the real QMK steno adapter and Rust engine.
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "steno/chord.h"
#include "steno/engine.h"

static char text[2048];
static size_t text_len;
static uint16_t taps[256];
static size_t tap_len;
static bool jis_host;
static bool alternate_available, alternate_selected;
static unsigned alternate_attempts;
bool mejiro_steno_select_alternate_system(void) {
    // Selection must precede the IME key, including when no system exists.
    assert(tap_len == 0);
    ++alternate_attempts;
    alternate_selected = alternate_available;
    return alternate_selected;
}
static mejiro_hid_event replayed[MEJIRO_EXTERNAL_CAPACITY];
static size_t replay_offsets[MEJIRO_EXTERNAL_CAPACITY], replay_count;
static size_t replay_operation_offsets[MEJIRO_EXTERNAL_CAPACITY];
static unsigned replay_begins, replay_ends;
void mejiro_keyboard_replay_begin(void) { ++replay_begins; }
void mejiro_keyboard_replay_end(void) { ++replay_ends; }
void mejiro_keyboard_replay_event(const mejiro_hid_event *event) {
    assert(replay_count < MEJIRO_EXTERNAL_CAPACITY);
    replay_offsets[replay_count] = text_len;
    replay_operation_offsets[replay_count] = tap_len;
    replayed[replay_count++] = *event;
    // Transport feedback must not append the replay to a recorder again.
    mejiro_engine_record(*event);
}

bool mejiro_steno_jis_host(void) { return jis_host; }
void mejiro_steno_tap(uint16_t key) {
    assert(tap_len < sizeof(taps) / sizeof(*taps));
    taps[tap_len++] = key;
}
void send_char(char c) {
    assert(text_len + 1 < sizeof(text));
    text[text_len++] = c;
    text[text_len] = '\0';
}

static void clear_output(void) {
    text_len = tap_len = 0;
    text[0] = '\0';
    replay_count = replay_begins = replay_ends = 0;
}

static void chord(const uint16_t *keys, size_t count) {
    for (size_t i = 0; i < count; ++i) mejiro_on_press(keys[i]);
    for (size_t i = 0; i < count; ++i) mejiro_on_release(keys[i]);
}
#define CHORD(...) do { const uint16_t keys[] = {__VA_ARGS__}; chord(keys, sizeof(keys) / sizeof(*keys)); } while (0)

static void test_one_hand_mode(void) {
    mejiro_engine_init();
    clear_output();
    // Actual physical symbol keys: # on the left, * on the right.
    CHORD(STN_N1, STN_S1, STN_TL, STN_KL, STN_WL, STN_PL,
          STN_HL, STN_RL, STN_ST1, STN_N3, STN_A, STN_O);
    assert(text_len == 0 && tap_len == 0);
    CHORD(STN_N1, STN_KL, STN_RL, STN_ST1); // #KAU- = こと
    assert(strcmp(text, "koto") == 0);
    clear_output();
    CHORD(STN_N1, STN_N3); // #n = しない, not macro replay
    assert(strcmp(text, "shinai") == 0);
    clear_output();
    CHORD(STN_SR, STN_LR, STN_GR, STN_BR, STN_PR, STN_FR,
          STN_RR, STN_ST3, STN_N4, STN_U, STN_E, STN_DR);
    assert(text_len == 0 && tap_len == 0);
    CHORD(STN_ST3); // -U = う, not Undo
    assert(strcmp(text, "u") == 0 && tap_len == 0);
    clear_output();
    mejiro_engine_process("#-n", 3); // return to both hands without leaving Romaji
    CHORD(STN_KL, STN_RL);
    assert(strcmp(text, "ka") == 0);
    clear_output();
    CHORD(STN_SR, STN_LR, STN_GR, STN_BR, STN_PR, STN_FR,
          STN_RR, STN_ST3, STN_N4, STN_U, STN_E, STN_DR);
    clear_output();
    CHORD(STN_N4); // right n is Space in one-hand mode
    assert(text_len == 0 && tap_len == 1 && taps[0] == KC_SPC);
    clear_output();
    CHORD(STN_KL, STN_RL, STN_LR, STN_RR); // KA-TA returns to both hands
    assert(strcmp(text, "kata") == 0);
    clear_output();
    CHORD(STN_ST3); // Undo is restored immediately
    assert(text_len == 0 && tap_len == 2 && taps[0] == KC_BSPC && taps[1] == KC_BSPC);
    clear_output();
    // Full all-key chord returns to both hands without adding an undo entry.
    CHORD(STN_N1, STN_S1, STN_TL, STN_KL, STN_WL, STN_PL,
          STN_HL, STN_RL, STN_ST1, STN_N3, STN_A, STN_O);
    CHORD(STN_N1, STN_S1, STN_TL, STN_KL, STN_WL, STN_PL,
          STN_HL, STN_RL, STN_ST1, STN_N3, STN_A, STN_O,
          STN_SR, STN_LR, STN_GR, STN_BR, STN_PR, STN_FR,
          STN_RR, STN_ST3, STN_N4, STN_U, STN_E, STN_DR);
    assert(text_len == 0 && tap_len == 0);
    CHORD(STN_ST3);
    assert(text_len == 0 && tap_len == 1 && taps[0] == KC_BSPC);
}

int main(void) {
    test_one_hand_mode();
    // Both with and without an alternate system: select first, then one IME key.
    for (unsigned available = 0; available < 2; ++available) {
        alternate_available = available != 0;
        for (unsigned host = 0; host < 2; ++host) {
            jis_host = host != 0;
            mejiro_engine_init();
            clear_output();
            alternate_attempts = 0;
            alternate_selected = false;
            mejiro_engine_process("#-t", 3);
            assert(alternate_attempts == 0);
            assert(text_len == 0 && tap_len == 1 && taps[0] == KC_LNG1);
            clear_output();
            mejiro_engine_process("#-k", 3);
            assert(text_len == 0 && tap_len == 1 && taps[0] == KC_LNG2);
            assert(alternate_attempts == 1 && alternate_selected == alternate_available);
        }
    }
    alternate_available = false;
    jis_host = false;
    clear_output();
    // The QMK adapter must consume the revised shared Rust vowel definitions.
    const struct { const char *stroke; const char *expected; } vowels[] = {
        {"Yk-", "wha"}, {"STKNYk-", "va"}, {"STKNIUntk-", "thu-"},
        {"STKNYIUntk-", "vulyu-"}, {"TNYIUntk-", "dwu-"},
        {"TNYIAUntk-", "dulo-"},
    };
    for (size_t i = 0; i < sizeof(vowels) / sizeof(*vowels); ++i) {
        mejiro_engine_init();
        clear_output();
        mejiro_engine_process(vowels[i].stroke, (uint8_t)strlen(vowels[i].stroke));
        assert(strcmp(text, vowels[i].expected) == 0);
    }
    clear_output();
    // STN without a vowel contributes no kana; its extra sound still emits.
    mejiro_engine_init();
    CHORD(STN_S1, STN_TL, STN_WL); // STN-
    assert(text_len == 0 && tap_len == 0);
    CHORD(STN_S1, STN_TL, STN_WL, STN_A, STN_O); // STNtk-
    assert(strcmp(text, "ltsu") == 0);
    clear_output();
    mejiro_engine_init();
    CHORD(STN_S1, STN_TL, STN_WL, STN_N3, STN_A, STN_O); // STNntk-
    assert(strcmp(text, "-") == 0);
    clear_output();
    mejiro_engine_init();
    CHORD(STN_KL, STN_RL, STN_N3, STN_A, STN_O,
          STN_SR, STN_LR, STN_BR, STN_U, STN_E); // KAntk-STNtk
    assert(strcmp(text, "ka-ltsu") == 0);
    clear_output();
    // Context resets preserve recording. Only explicit controls stop it.
    mejiro_engine_init();
    mejiro_engine_process("#n-*", 4);
    mejiro_engine_process("#t-*", 4);
    mejiro_engine_process("KA-", 3);
    extern bool mejiro_qmk_is_recording(size_t);
    assert(mejiro_qmk_is_recording(0) && mejiro_qmk_is_recording(1));
    mejiro_reset_state();
    assert(mejiro_qmk_is_recording(0) && mejiro_qmk_is_recording(1));
    mejiro_engine_process("#-*", 3);
    mejiro_engine_process("#-*", 3);
    for (size_t i = 0; i < 7; ++i) assert(!mejiro_qmk_is_recording(i));
    clear_output();
    mejiro_engine_process("#n", 2);
    assert(strcmp(text, "ka") == 0);
    clear_output();
    mejiro_engine_process("#t", 2);
    assert(strcmp(text, "ka") == 0);
    clear_output();
    mejiro_engine_init();
    CHORD(STN_ST3);                        // -U with no history erases two units
    assert(tap_len == 2 && taps[0] == KC_BSPC && taps[1] == KC_BSPC);
    clear_output();
    CHORD(STN_KL, STN_RL);                 // KA-
    assert(strcmp(text, "ka") == 0);
    clear_output();
    CHORD(STN_N1);                         // #- repeats the resolved output
    assert(strcmp(text, "ka") == 0);
    clear_output();
    CHORD(STN_ST3);                        // -U undoes the repeat
    assert(tap_len == 1 && taps[0] == KC_BSPC);
    clear_output();
    CHORD(STN_ST3);                        // -U undoes KA-
    assert(tap_len == 1 && taps[0] == KC_BSPC);
    clear_output();
    CHORD(STN_ST3);                        // history exhausted: two Backspaces
    assert(tap_len == 2 && taps[0] == KC_BSPC && taps[1] == KC_BSPC);
    clear_output();

    // KA-TAtk renders "かた" and carries its final small-tsu. The first
    // Undo must erase two kana, while the next stroke owns the deferred っ.
    mejiro_engine_init();
    mejiro_engine_process("KA-TAtk", 7);
    assert(strcmp(text, "kata") == 0);
    clear_output();
    mejiro_engine_process("-U", 2);
    assert(tap_len == 2 && taps[0] == KC_BSPC && taps[1] == KC_BSPC);
    clear_output();
    mejiro_engine_init();
    mejiro_engine_process("KA-TAtk", 7);
    clear_output();
    mejiro_engine_process("TA-", 3);
    assert(strcmp(text, "tta") == 0);
    clear_output();
    mejiro_engine_process("-U", 2);
    assert(tap_len == 2 && taps[0] == KC_BSPC && taps[1] == KC_BSPC);
    clear_output();
    mejiro_engine_process("-U", 2);
    assert(tap_len == 2 && taps[0] == KC_BSPC && taps[1] == KC_BSPC);
    clear_output();

    CHORD(STN_N3, STN_DR);                 // n-*
    CHORD(STN_O, STN_E);                   // k-k after a nominalizable word
    assert(strcmp(text, "shiteirunoga") == 0);
    CHORD(STN_ST3);                        // -U
    assert(tap_len == 2 && taps[0] == KC_BSPC && taps[1] == KC_BSPC);
    clear_output();
    CHORD(STN_O, STN_E);
    assert(strcmp(text, "noga") == 0);     // undo restored grammar
    clear_output();

    mejiro_engine_init();
    CHORD(STN_N1, STN_N3, STN_DR);        // #n-* starts recording
    CHORD(STN_N3, STN_DR);
    CHORD(STN_O, STN_E);
    CHORD(STN_N1, STN_DR);                // #-* stops recording
    clear_output();
    CHORD(STN_N1, STN_N3);                // #n replays recorded output
    assert(strcmp(text, "shiteirunoga") == 0);
    CHORD(STN_ST3);                        // -U erases six kana units
    assert(tap_len == 6);
    for (size_t i = 0; i < tap_len; ++i) assert(taps[i] == KC_BSPC);
    clear_output();

    mejiro_engine_init();
    CHORD(STN_LR, STN_PR, STN_FR);        // -TYI: （） then Left
    assert(strcmp(text, "()") == 0);
    assert(tap_len == 1 && taps[0] == KC_LEFT);
    clear_output();
    jis_host = true;
    CHORD(STN_LR, STN_PR, STN_FR);
    assert(text_len == 0);
    assert(tap_len == 3 && taps[0] == KC_LPRN && taps[1] == KC_RPRN && taps[2] == KC_LEFT);
    clear_output();
    jis_host = false;

    mejiro_engine_init();
    mejiro_first_up_chord_send = false;
    CHORD(STN_KL, STN_RL);
    assert(strcmp(text, "ka") == 0);
    clear_output();
    mejiro_engine_init();
    mejiro_engine_process("#n-*", 4);
    mejiro_engine_process("KA-", 3);
    mejiro_engine_reset();
    mejiro_engine_record((mejiro_hid_event){.usage = 6, .page = 7, .mods = 1, .kind = 1});
    mejiro_engine_record((mejiro_hid_event){.usage = 6, .page = 7, .mods = 1, .kind = 2});
    assert(replay_count == 0);
    assert(mejiro_engine_flush_keyboard());
    assert(replay_count == 0); // Import does not resend already-typed keys.
    mejiro_engine_process("KI-", 3);
    mejiro_engine_process("#-*", 3);
    clear_output();
    mejiro_engine_process("#n", 2);
    assert(strcmp(text, "kaki") == 0);
    assert(replay_count == 2 && replay_offsets[0] == 2 && replay_offsets[1] == 2);
    assert(replayed[0].usage == 6 && replayed[0].mods == 1);
    assert(replay_begins == 1 && replay_ends == 1);
    clear_output();
    mejiro_engine_process("#n", 2);
    assert(replay_count == 2); // No feedback loop on repeated replay.
    mejiro_engine_process("#t-*", 4);
    mejiro_engine_process("#n", 2);
    mejiro_engine_process("#-*", 3);
    clear_output();
    mejiro_engine_process("#t", 2);
    assert(replay_count == 2); // Other active slots get one copy, never feedback.

    mejiro_engine_init();
    mejiro_engine_process("#n-*", 4);
    mejiro_engine_process("-n", 2);
    mejiro_engine_record((mejiro_hid_event){.usage = 6, .page = 7, .kind = 1});
    mejiro_engine_record((mejiro_hid_event){.usage = 6, .page = 7, .kind = 2});
    mejiro_engine_process("-n", 2);
    mejiro_engine_process("#-*", 3);
    clear_output();
    mejiro_engine_process("#n", 2);
    assert(tap_len == 2 && replay_count == 2);
    assert(replay_operation_offsets[0] == 1 && replay_operation_offsets[1] == 1);

    mejiro_engine_init();
    mejiro_engine_process("#n-*", 4);
    for (unsigned i = 0; i <= MEJIRO_EXTERNAL_CAPACITY; ++i)
        mejiro_engine_record((mejiro_hid_event){.usage = 6, .page = 7, .kind = 1});
    assert(mejiro_engine_recording_failed() && !mejiro_qmk_is_recording(0));
    mejiro_engine_process("#n-*", 4); // Stop the failed slot, do not restart it.
    assert(!mejiro_qmk_is_recording(0));
    clear_output();
    mejiro_engine_process("#n", 2);
    assert(!replay_count && !text_len && !tap_len);
    mejiro_engine_init();
    mejiro_engine_process("#n-*", 4);
    for (unsigned i = 0; i < 300; ++i) mejiro_engine_process("KA-", 3);
    assert(text_len == 600); // A full text recorder must not eat live steno.
    assert(mejiro_engine_recording_failed() && !mejiro_qmk_is_recording(0));
    mejiro_engine_init();
    mejiro_rule rule = {.kind = 0, .key_len = 3, .value_len = 3};
    memcpy(rule.key, "KA-", 3);
    memcpy(rule.value, "き", 3);
    assert(mejiro_qmk_rule_set(&rule) == 0 && mejiro_qmk_rule_count() == 1);
    mejiro_rule loaded = {0};
    assert(mejiro_qmk_rule_get(0, &loaded) == 0 && memcmp(loaded.value, "き", 3) == 0);
    clear_output();
    mejiro_engine_process("KA-", 3);
    assert(strcmp(text, "ki") == 0);
    assert(mejiro_qmk_rule_hide(0, (const uint8_t *)"KA-", 3) == 0);
    clear_output();
    mejiro_engine_process("KA-", 3);
    assert(strcmp(text, "ka") == 0);
    assert(mejiro_qmk_rule_clear(0, (const uint8_t *)"KA-", 3) == 0);
    assert(mejiro_qmk_rule_count() == 0);
    puts("steno Rust engine + QMK adapter, mixed batch recording and overflow OK");
    return 0;
}
