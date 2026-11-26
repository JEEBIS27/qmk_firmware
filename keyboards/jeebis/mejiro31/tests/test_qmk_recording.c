// Real unified keyboard adapter + Rust recorder; only host USB IO is mocked.
#define MJI_TEST_REAL_RECORDING
#define main ordinary_adapter_tests_main
#include "test_qmk_adapter.c"
#undef main

static char emitted_text[2048];
static size_t emitted_len;
void send_char(char c) {
    assert(emitted_len + 1 < sizeof(emitted_text));
    emitted_text[emitted_len++] = c;
    emitted_text[emitted_len] = '\0';
}
extern bool mejiro_qmk_is_recording(size_t);
extern int32_t mejiro_qmk_replayed_external(mejiro_recorded_event *, size_t);

static void test_real_jis_usages(void) {
    setup();
    const char fixture[] = "ろーをがぱ、。";
    mejiro_rule rule = {.kind = 0, .key_len = 3, .value_len = sizeof(fixture) - 1};
    memcpy(rule.key, "KA-", 3);
    memcpy(rule.value, fixture, sizeof(fixture) - 1);
    assert(mejiro_qmk_rule_set(&rule) == 0);
    mejiro_engine_set_output(1);
    assert(!input.layout.jis_host); // Built-in kana uses JIS independent of keyboard mode.
    set_mods(MOD_LSFT);
    mejiro_engine_process("KA-", 3);
    assert(presses[KC_INT1] == 1 && pressed_mods[KC_INT1] == 0);
    assert(presses[KC_INT3] == 1 && pressed_mods[KC_INT3] == MOD_LSFT);
    assert(presses[KC_0] == 1 && pressed_mods[KC_0] == MOD_LSFT);
    assert(presses[KC_T] == 1 && pressed_mods[KC_T] == 0);
    assert(presses[KC_LBRC] == 1 && pressed_mods[KC_LBRC] == 0); // JP @
    assert(presses[KC_F] == 1 && pressed_mods[KC_F] == 0);
    assert(presses[KC_RBRC] == 1 && pressed_mods[KC_RBRC] == 0); // JP [
    assert(presses[KC_COMM] == 1 && pressed_mods[KC_COMM] == MOD_LSFT);
    assert(presses[KC_DOT] == 1 && pressed_mods[KC_DOT] == MOD_LSFT);
    assert(get_mods() == MOD_LSFT);
    set_mods(0);
    mejiro_engine_process("-NY", 3);
    mejiro_engine_process("-KNA", 4);
    assert(presses[KC_SLSH] == 1 && pressed_mods[KC_SLSH] == MOD_LSFT);
    assert(presses[KC_RBRC] == 2 && pressed_mods[KC_RBRC] == MOD_LSFT);
    assert(presses[KC_NUHS] == 1 && pressed_mods[KC_NUHS] == MOD_LSFT);
    all_up();
}
int main(void) {
    test_real_jis_usages();
    setup();
    mejiro_mode = true;
    steno_mode = true;
    switch_context();
    mejiro_engine_process("#n-*", 4);
    mejiro_engine_process("KA-", 3);
    event(0, 29, true);
    event(5, 29, false);
    assert(!steno_mode && mejiro_qmk_is_recording(0));
    event(10, 41, true); // Ctrl
    event(15, 15, true); // C
    tick(55);
    assert(native_keys[KC_C]);
    event(60, 41, false); // Modifier first: C still needs a recorded key-up.
    event(65, 15, false);
    unsigned typed_c = presses[KC_C];
    event(70, 29, true);
    event(75, 29, false);
    assert(steno_mode && mejiro_qmk_is_recording(0));
    assert(presses[KC_C] == typed_c); // Reentry imports, never sends again.
    mejiro_engine_process("KI-", 3);
    mejiro_engine_process("#-*", 3);
    assert(!mejiro_qmk_is_recording(0));
    emitted_len = 0;
    emitted_text[0] = '\0';
    mejiro_engine_process("#n", 2);
    assert(strcmp(emitted_text, "kaki") == 0);
    assert(presses[KC_C] == typed_c + 1);
    assert(pressed_mods[KC_C] == MOD_LCTL);
    mejiro_recorded_event saved[MEJIRO_EXTERNAL_CAPACITY];
    int32_t count = mejiro_qmk_replayed_external(saved, MEJIRO_EXTERNAL_CAPACITY);
    bool saw_up = false;
    for (int32_t i = 0; i < count; ++i)
        if (saved[i].event.page == 7 && saved[i].event.usage == KC_C && saved[i].event.kind == 2) {
            saw_up = true;
            assert(!saved[i].event.mods);
        }
    assert(saw_up);
    all_up();
    assert(!consumer_usage && !system_usage);

    // A recording ending with a key held still releases replay-owned keys.
    mejiro_engine_process("#t-*", 4);
    mejiro_engine_record((mejiro_hid_event){.usage = KC_A, .page = 7, .mods = MOD_LALT, .kind = 1});
    mejiro_engine_process("#-*", 3);
    unsigned before_a = presses[KC_A];
    set_mods(MOD_LSFT);
    mejiro_engine_process("#t", 2);
    assert(presses[KC_A] == before_a + 1 && pressed_mods[KC_A] == MOD_LALT);
    assert(!native_keys[KC_A] && native_mods == MOD_LSFT);
    set_mods(0);
    all_up();

    // A live key owned by the keyboard engine is restored after raw replay.
    event(80, 29, true);
    event(85, 29, false);
    event(90, 1, true);
    tick(130);
    assert(native_keys[KC_Q]);
    mejiro_engine_process("#t", 2);
    assert(native_keys[KC_Q] && !native_keys[KC_A]);
    event(135, 1, false);
    event(140, 29, true);
    event(145, 29, false);
    assert(steno_mode);
    all_up();

    // Overflow aborts capture without blocking subsequent real key input.
    mejiro_engine_process("#n-*", 4);
    event(200, 29, true);
    event(205, 29, false);
    for (unsigned i = 0; i < 80; ++i) {
        event(210 + i * 100, 0, true);
        event(215 + i * 100, 0, false);
    }
    assert(mejiro_engine_recording_failed() && !mejiro_qmk_is_recording(0));
    assert(presses[KC_GRV] == 80);
    all_up();
    puts("real QMK/Rust mixed recording: mode roundtrip, no duplicate IO, cleanup, live overflow OK");
}
