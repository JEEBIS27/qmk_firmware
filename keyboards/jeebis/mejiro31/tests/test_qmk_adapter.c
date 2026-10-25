// SPDX-License-Identifier: GPL-2.0-or-later
#include <assert.h>
#include <stdio.h>
#include "keymap_japanese.h"
#ifdef MJI_TEST_EN
#    include "../keymaps/en_default/keymap.c"
#else
#    include "../keymaps/jp_default/keymap.c"
#endif
static uint8_t pressed_mods[256];

struct stub_keymap_config keymap_config;
static uint8_t            native_mods;
static bool               native_keys[256];
static uint32_t           now;
static unsigned           presses[256], steno_events;
static unsigned           steno_packets;
static int                steno_pending;
static uint16_t           dynamic_maps[12][4][12];
static uint32_t           settings_word;
static unsigned           settings_writes;
static uint16_t consumer_usage, system_usage;
void host_consumer_send(uint16_t usage) { consumer_usage = usage; }
void host_system_send(uint16_t usage) { system_usage = usage; }
uint32_t eeconfig_read_user(void) { return settings_word; }
void eeconfig_update_user(uint32_t value) { settings_word = value; ++settings_writes; }
void                      set_mods(uint8_t m) {
    native_mods = m;
}
void set_weak_mods(uint8_t m) {
    (void)m;
}
void clear_oneshot_mods(void) {}
void set_oneshot_mods(uint8_t m) {
    (void)m;
}
void send_keyboard_report(void) {}
void clear_keyboard(void) {
    memset(native_keys, 0, sizeof(native_keys));
    native_mods = 0;
}
void register_code(uint8_t c) {
    native_keys[c] = true;
    presses[c]++;
    pressed_mods[c] = native_mods;
}
void unregister_code(uint8_t c) {
    native_keys[c] = false;
}
void tap_code16(uint16_t c) {
    register_code(c);
    unregister_code(c);
}
uint8_t get_mods(void) {
    return native_mods;
}
uint8_t get_weak_mods(void) {
    return 0;
}
uint8_t get_oneshot_mods(void) {
    return 0;
}
void del_mods(uint8_t m) {
    native_mods &= ~m;
}
void del_weak_mods(uint8_t m) {
    (void)m;
}
void layer_clear(void) {}
void default_layer_set(layer_state_t l) {
    (void)l;
}
void     reset_keyboard(void) {}
uint32_t timer_read32(void) {
    return now;
}
os_variant_t detected_host_os(void) {
    return OS_LINUX;
}
void steno_set_mode(steno_mode_t m) {
    (void)m;
}
bool process_steno(uint16_t c, keyrecord_t *r) {
    (void)c;
    (void)r;
    steno_events++;
    steno_pending += r->event.pressed ? 1 : -1;
    assert(steno_pending >= 0);
    uint8_t chord[6] = {0};
    if (!r->event.pressed && !steno_pending && send_steno_chord_user(STENO_MODE_GEMINI, chord)) ++steno_packets;
    return false;
}
bool is_stn_key(uint16_t kc) {
    return kc >= STN_N1 && kc <= STN_ZR;
}
void mejiro_reset_state(void) {
#ifdef MJI_TEST_REAL_RECORDING
    mejiro_engine_reset();
#endif
}
#ifndef MJI_TEST_REAL_RECORDING
void mejiro_engine_init(void) {}
void mejiro_engine_reset(void) {}
void mejiro_engine_record(mejiro_hid_event event) { (void)event; }
bool mejiro_engine_flush_keyboard(void) { return true; }
bool mejiro_engine_recording_failed(void) { return false; }
#endif
void mejiro_on_press(uint16_t c) {
    (void)c;
    steno_events++;
}
void mejiro_on_release(uint16_t c) {
    (void)c;
    steno_events++;
}
uint16_t keycode_at_keymap_location(uint8_t l, uint8_t r, uint8_t c) {
    assert(l < 12 && r < 4 && c < 12);
    return dynamic_maps[l][r][c];
}
static void setup(void) {
    settings_word = UINT32_MAX;
    settings_writes = 0;
    memset(presses, 0, sizeof(presses));
    clear_keyboard();
    memcpy(dynamic_maps, keymaps, sizeof(dynamic_maps));
    memset(held_codes, 0, sizeof(held_codes));
    memset(refs, 0, sizeof(refs));
    steno_mode   = false;
    steno_held   = 0;
    mejiro_mode  = MJI_DEFAULT_MEJIRO;
    host_mods    = 0;
    steno_events = 0;
    steno_pending = 0;
    steno_packets = 0;
    gemini_held = 0;
    shortcut_active = discard_steno_send = false;
    keyboard_post_init_user();
    assert(keymap_config.nkro);
    // Tests select their own Alt maps; user-selected firmware defaults must
    // not change the expected HID keys in unrelated input tests.
    mji_layout_config_t config = input.layout;
    config.maps[0] = config.maps[1] = (mji_layout_map_t)MJI_LAYOUT_MAP(mji_layout_qwerty);
    assert(select_layout(&config));
}
static void event(uint32_t time, uint8_t p, bool pressed) {
    now           = time;
    keyrecord_t r = {.event = {.key = {.row = p / 12, .col = p % 12}, .pressed = pressed}};
    assert(!pre_process_record_user(0, &r));
}
static void tick(uint32_t time) {
    now = time;
    matrix_scan_user();
}
static void all_up(void) {
    assert(!native_mods);
    assert(!consumer_usage && !system_usage);
    for (unsigned i = 0; i < 256; i++)
        assert(!native_keys[i]);
}
static void test_real_geometry(void) {
    setup();
    event(0, 4, true);
    event(5, 5, true);
    event(10, 16, true);
    event(15, 17, true);
    assert(native_keys[KC_F] && native_keys[KC_G] && !native_keys[KC_ESC]);
    event(20, 4, false);
    event(21, 16, false);
    event(22, 5, false);
    event(23, 17, false);
    all_up();
}
static void test_dynamic_combo_tap_hold(void) {
    setup();
    dynamic_maps[0][2][2] = MJI_STICKY;
    dynamic_maps[6][2][2] = KC_LALT;
    event(0, 3, true);
    event(5, 15, true);
    assert(native_mods == MOD_LALT);
    event(10, 3, false);
    event(11, 15, false);
    assert(input.sticky_waiting);
    all_up();
    event(20, 2, true);
    event(25, 14, true);
    assert(native_keys[KC_S] && (native_mods & MOD_MASK_SHIFT));
    event(30, 2, false);
    event(31, 14, false);
    all_up();
}
static void test_00_uses_normal_zero_hold(void) {
    setup();
    dynamic_maps[0][0][0] = MJI_00;
    event(0, 0, true);
    assert(presses[KC_0] == 2 && native_keys[KC_0]);
    tick(1000);
    assert(presses[KC_0] == 2);
    event(1001, 0, false);
    all_up();
}
static void test_mode_and_language_clear(void) {
    setup();
    event(0, 28, true);
    event(5, 3, true);
    assert(native_mods & MOD_MASK_SHIFT);
    dynamic_maps[1][0][0] = KC_LNG1;
    event(10, 0, true);
    tick(45);
    assert(input.layout.language == MJI_JAPANESE && input.ignored);
    all_up();
    event(50, 28, false);
    event(51, 3, false);
    event(52, 0, false);
    tick(100);
    all_up();
    event(110, 29, true);
    event(120, 29, false);
    assert(steno_mode);
    all_up();
    event(130, 1, true);
    event(135, 1, false);
    assert(steno_events == 2);
    all_up();
    event(140, 29, true);
    event(150, 29, false);
    assert(!steno_mode);
    all_up();
}
static void test_steno_entry_ime_on(void) {
    for (unsigned use_mejiro = 0; use_mejiro < 2; ++use_mejiro) {
        setup();
        mejiro_mode = use_mejiro;
        // Existing Ctrl must be released before the IME command is sent.
        event(0, 41, true);
        event(5, 29, true);
        assert(native_mods & MOD_LCTL);
        event(10, 29, false);
        assert(steno_mode);
        unsigned expected = !MJI_EN_PROFILE && use_mejiro;
        assert(presses[KC_LNG1] == expected);
        assert(!pressed_mods[KC_LNG1]);
        all_up();
        event(15, 41, false);
        event(20, 29, true);
        event(25, 29, false);
        assert(!steno_mode && presses[KC_LNG1] == expected);
        event(30, 29, true);
        event(35, 29, false);
        assert(steno_mode && presses[KC_LNG1] == 2 * expected);
        assert(presses[KC_LNG2] == 1);
        all_up();
    }
}
static void test_keyboard_language_restored(void) {
    static const mji_layout_entry_t en[] = {{KC_Q, {KC_A, 0}, {KC_A, 0}}};
    static const mji_layout_entry_t jp[] = {{KC_Q, {KC_B, 0}, {KC_B, 0}}};
    for (unsigned language = MJI_ENGLISH; language <= MJI_JAPANESE; ++language) {
        for (unsigned use_mejiro = 0; use_mejiro < 2; ++use_mejiro) {
            setup();
            mejiro_mode = use_mejiro;
            mji_layout_config_t config = input.layout;
            config.maps[0] = (mji_layout_map_t){en, 1};
            config.maps[1] = (mji_layout_map_t){jp, 1};
            config.language = language;
            assert(select_layout(&config));
            event(0, 29, true);
            event(5, 29, false);
            assert(steno_mode);
            // A language change within steno must not overwrite the saved
            // keyboard language, even if it selects the opposite profile.
            dynamic_maps[5][0][0] = language == MJI_JAPANESE ? KC_LNG2 : KC_LNG1;
            event(10, 0, true);
            event(15, 0, false);
            assert(input.layout.language != language);
            unsigned before_on = presses[KC_LNG1], before_off = presses[KC_LNG2];
            event(20, 29, true);
            event(25, 29, false);
            assert(!steno_mode && input.layout.language == language);
            assert(presses[KC_LNG1] == before_on + (language == MJI_JAPANESE));
            assert(presses[KC_LNG2] == before_off + (language == MJI_ENGLISH));
            assert(!pressed_mods[KC_LNG1] && !pressed_mods[KC_LNG2]);
            event(30, 1, true);
            tick(70);
            assert(native_keys[language == MJI_JAPANESE ? KC_B : KC_A]);
            event(75, 1, false);
            all_up();
        }
    }
}
static void test_same_key_multiple_owners(void) {
    setup();
    dynamic_maps[0][0][0] = KC_A;
    dynamic_maps[0][3][4] = KC_A;
    dynamic_maps[6][3][4] = 0;
    event(0, 0, true);
    event(5, 40, true);
    assert(native_keys[KC_A]);
    event(10, 0, false);
    assert(native_keys[KC_A]);
    event(15, 40, false);
    all_up();
}
static void test_steno_number_layer(void) {
    setup();
    event(0, 29, true);
    event(10, 29, false);
    assert(steno_mode);
    event(20, 29, true);
    event(25, 2, true);
    tick(65);
    assert(native_keys[KC_1]);
    assert(!steno_events);
    event(70, 2, false);
    event(75, 29, false);
    assert(steno_mode);
    all_up();
}
static void test_00_with_another_zero_owner(void) {
    setup();
    dynamic_maps[0][0][0] = KC_0;
    dynamic_maps[0][3][4] = MJI_00;
    dynamic_maps[6][3][4] = KC_NO;
    event(0, 0, true);
    event(5, 40, true);
    assert(native_keys[KC_0]);
    event(10, 40, false);
    assert(native_keys[KC_0]);
    event(15, 0, false);
    all_up();
}
static void test_jis_shifted_symbol(void) {
    setup();
    mji_layout_config_t config = input.layout;
    config.jis_host            = true;
    assert(select_layout(&config));
    dynamic_maps[1][0][0] = KC_AT;
    event(0, 28, true);
    event(1, 0, true);
    assert(native_keys[JP_AT]);
    assert(!(pressed_mods[JP_AT] & MOD_MASK_SHIFT));
    event(2, 0, false);
    event(3, 28, false);
    all_up();
}

static void test_jis_repeat_and_next_key_mods(void) {
    setup();
    mji_layout_config_t config = input.layout;
    config.jis_host            = true;
    assert(select_layout(&config));
    dynamic_maps[1][0][0] = KC_AT;
    event(0, 28, true);
    event(1, 0, true);
    tick(100);
    assert(native_keys[JP_AT] && !(native_mods & MOD_MASK_SHIFT));
    event(110, 2, true);
    tick(150);
    assert(native_keys[KC_W] && (native_mods & MOD_MASK_SHIFT));
    event(160, 0, false);
    assert(native_mods & MOD_MASK_SHIFT);
    event(161, 2, false);
    event(162, 28, false);
    all_up();
    setup();
    config          = input.layout;
    config.jis_host = true;
    assert(select_layout(&config));
    dynamic_maps[0][0][0] = KC_QUOT;
    event(0, 0, true);
    tick(100);
    assert(native_keys[KC_7] && (native_mods & MOD_MASK_SHIFT));
    event(110, 2, true);
    tick(150);
    assert(native_keys[KC_W] && !(native_mods & MOD_MASK_SHIFT));
    event(160, 0, false);
    assert(!(native_mods & MOD_MASK_SHIFT));
    event(161, 2, false);
    all_up();
}
static void test_alt_combo_snapshot_and_shortcut(void) {
    static const mji_layout_entry_t map[] = {{KC_D, {KC_H, 0}, {KC_J, 0}}, {KC_Q, {KC_P, 0}, {KC_P, 0}}};
    setup();
    mji_layout_config_t config = input.layout;
    config.maps[MJI_ENGLISH]   = (mji_layout_map_t){map, 2};
    assert(select_layout(&config));
    event(0, 3, true);
    event(5, 28, true);
    event(10, 15, true);
    assert(native_keys[KC_H] && !(pressed_mods[KC_H] & MOD_MASK_SHIFT));
    event(20, 3, false);
    event(21, 15, false);
    event(22, 28, false);
    all_up();
    event(30, 41, true);
    event(35, 1, true);
    tick(75);
    assert(native_keys[KC_Q] && !native_keys[KC_P]);
    event(80, 1, false);
    event(81, 41, false);
    all_up();
}
static void test_alt_language_switch_and_reset(void) {
    static const mji_layout_entry_t en[] = {{KC_Q, {KC_A, 0}, {KC_A, 0}}};
    static const mji_layout_entry_t jp[] = {{KC_Q, {KC_B, 0}, {KC_B, 0}}};
    setup();
    mji_layout_config_t config = input.layout;
    config.maps[0]             = (mji_layout_map_t){en, 1};
    config.maps[1]             = (mji_layout_map_t){jp, 1};
    assert(select_layout(&config));
    event(0, 1, true);
    tick(40);
    assert(native_keys[KC_A]);
    dynamic_maps[0][0][0] = KC_LNG1;
    event(50, 0, true);
    assert(input.layout.language == MJI_JAPANESE);
    all_up();
    tick(100);
    all_up();
    event(101, 1, false);
    event(102, 0, false);
    event(110, 1, true);
    tick(150);
    assert(native_keys[KC_B] && !native_keys[KC_A]);
    event(160, 1, false);
    all_up();
}
static void test_alt_tap_and_hold_strokes(void) {
    static const mji_layout_entry_t map[] = {{KC_A, {KC_D, 0}, {KC_D, 0}}, {KC_B, {KC_E, 0}, {KC_E, 0}}};
    setup();
    mji_layout_config_t config = input.layout;
    config.maps[0]             = (mji_layout_map_t){map, 2};
    assert(select_layout(&config));
    dynamic_maps[0][0][0] = KC_A;
    dynamic_maps[6][0][0] = KC_B;
    event(0, 0, true);
    assert(native_keys[KC_E] && !native_keys[KC_D]);
    event(10, 0, false);
    assert(presses[KC_D] == 1);
    all_up();
}
static void test_jis_sticky_symbol(void) {
    setup();
    mji_layout_config_t config = input.layout;
    config.jis_host            = true;
    assert(select_layout(&config));
    dynamic_maps[0][0][0] = MJI_STICKY;
    dynamic_maps[1][3][4] = KC_AT;
    dynamic_maps[7][3][4] = KC_NO;
    event(0, 0, true);
    event(1, 0, false);
    event(5, 40, true);
    tick(100);
    assert(native_keys[JP_AT] && !(native_mods & MOD_MASK_SHIFT));
    event(110, 2, true);
    tick(150);
    assert(native_keys[KC_W] && !(native_mods & MOD_MASK_SHIFT));
    event(160, 40, false);
    event(161, 2, false);
    all_up();
}
static void test_profile_defaults(void) {
    setup();
    assert(mejiro_mode == MJI_DEFAULT_MEJIRO);
#if MJI_EN_PROFILE
    assert(dynamic_maps[0][3][6] == KC_EXLM);
    assert(dynamic_maps[1][3][6] == KC_AMPR);
    assert(dynamic_maps[3][3][6] == KC_AMPR && dynamic_maps[9][3][6] == KC_NO);
#else
    assert(dynamic_maps[0][3][6] == KC_LNG2);
    assert(dynamic_maps[1][3][6] == KC_EXLM);
    assert(dynamic_maps[3][3][6] == KC_NO && dynamic_maps[9][3][6] == KC_LCTL);
#endif
}
static void test_steno_host_encoding(void) {
    setup();
    mji_layout_config_t config = input.layout;
    config.jis_host            = true;
    assert(select_layout(&config));
    set_mods(MOD_LSFT);
    mejiro_steno_tap(KC_AT);
    assert(presses[JP_AT] == 1 && !(pressed_mods[JP_AT] & MOD_MASK_SHIFT));
    assert(native_mods == MOD_LSFT);
    set_mods(0);
    all_up();
}
static void test_steno_shortcuts(void) {
    const uint16_t modifiers[] = {KC_LCTL, KC_RALT, KC_LGUI};
    for (unsigned mode = 0; mode < 2; ++mode)
        for (unsigned m = 0; m < 3; ++m) {
            setup();
            mejiro_mode = mode;
            steno_mode = true;
            switch_context();
            dynamic_maps[5][3][4] = KC_NO;
            dynamic_maps[11][3][4] = modifiers[m];
            event(0, 1, true); // Pending steno chord must be cancelled.
            event(5, 40, true);
            assert(shortcut_active && !steno_held && !steno_pending && !steno_packets);
            unsigned steno_before = steno_events;
            event(10, 15, true); // Physical C, not a steno W.
            tick(50);
            assert(native_keys[KC_C]);
            assert(pressed_mods[KC_C] == MOD_BIT(modifiers[m]));
            assert(steno_events == steno_before);
            event(55, 40, false); // Release modifier before C.
            event(60, 15, false);
            event(61, 1, false); // Suppressed pre-shortcut steno release.
            assert(!shortcut_active && !input.physical);
            all_up();
            event(70, 1, true);
            event(75, 1, false);
            assert(steno_events == steno_before + 2);
            if (!mode) assert(steno_packets == 1 && !steno_pending);
            all_up();
        }
    setup();
    steno_mode = true;
    switch_context();
    dynamic_maps[5][3][4] = KC_NO;
    dynamic_maps[11][3][4] = KC_LSFT;
    event(0, 40, true);
    assert(!shortcut_active);
    event(5, 1, true);
    event(10, 1, false);
    assert(steno_events == 2);
    event(15, 40, false);
    all_up();
    // Default geometry: hold Number, hold its GUI key, release Number,
    // then type a shortcut without requiring a custom steno modifier map.
    setup();
    steno_mode = true;
    switch_context();
    event(0, 29, true);
    event(5, 12, true);
    event(10, 29, false);
    assert(shortcut_active && steno_mode);
    event(15, 15, true);
    tick(55);
    assert(native_keys[KC_C] && (native_mods & MOD_MASK_GUI));
    event(60, 15, false);
    event(65, 12, false);
    assert(!shortcut_active && steno_mode);
    all_up();
    puts("adapter steno shortcuts, cancelled chords and release routing OK");
}
static void test_saved_settings_and_platform_keys(void) {
    setup();
    assert(settings_writes == 1);
    custom_action(MJI_JIS, true);
    custom_action(MJI_ALT, true);
    custom_action(MJI_MEJIRO, true);
    uint32_t saved = settings_word;
    unsigned writes = settings_writes;
    keyboard_post_init_user();
    assert(input.layout.jis_host == !MJI_DEFAULT_JIS);
    assert(!input.layout.alt_enabled && mejiro_mode == !MJI_DEFAULT_MEJIRO);
    assert(settings_word == saved && settings_writes == writes);
    save_settings();
    assert(settings_writes == writes);
    settings_word = 7; // Legacy flags have no matching version marker.
    keyboard_post_init_user();
    assert(input.layout.jis_host == MJI_DEFAULT_JIS);
    assert(input.layout.alt_enabled && mejiro_mode == MJI_DEFAULT_MEJIRO);

    for (unsigned end = 0; end < 2; ++end)
        for (unsigned command = 0; command < 2; ++command)
            for (unsigned shift = 0; shift < 2; ++shift) {
                setup();
                mac = true;
                dynamic_maps[0][0][0] = dynamic_maps[1][0][0] = end ? KC_END : KC_HOME;
                if (command) event(0, 41, true);
                if (shift) event(1, 28, true);
                event(2, 0, true);
                uint16_t arrow = end ? (command ? KC_DOWN : KC_RIGHT) : (command ? KC_UP : KC_LEFT);
                assert(native_keys[arrow] && (native_mods & MOD_MASK_GUI));
                assert(!!(native_mods & MOD_MASK_SHIFT) == shift);
                assert(!native_keys[KC_HOME] && !native_keys[KC_END]);
                tick(1000); // Retain the arrow for ordinary OS repeat.
                assert(native_keys[arrow]);
                event(1001, 0, false);
                assert(!native_keys[arrow]);
                if (shift) event(1002, 28, false);
                if (command) event(1003, 41, false);
                all_up();
            }
    setup();
    dynamic_maps[0][0][0] = KC_HOME;
    event(0, 0, true);
    assert(native_keys[KC_HOME] && !native_mods);
    event(1, 0, false);
    all_up();
    setup();
    mac = true;
    mejiro_steno_tap(LSFT(KC_HOME));
    assert(presses[KC_LEFT] == 1 && pressed_mods[KC_LEFT] == (MOD_LGUI | MOD_LSFT));
    assert(!native_mods);
    mejiro_steno_tap(LCTL(KC_ENT));
    assert(pressed_mods[KC_ENT] == MOD_LCTL); // Keep existing steno command semantics.
    all_up();

    setup();
    mji_layout_config_t config = input.layout;
    config.jis_host = true;
    assert(select_layout(&config));
    event(0, 40, true); // Alt
    event(1, 0, true);  // Grave / Zenkaku-Hankaku
    assert(native_keys[KC_GRV] && !native_keys[JP_AT]);
    assert(native_mods == MOD_LALT);
    event(2, 0, false);
    event(3, 40, false);
    all_up();
    puts("adapter settings reboot, Mac navigation, JIS Alt+grave OK");
}
int main(void) {
    test_steno_shortcuts();
    test_saved_settings_and_platform_keys();
    test_profile_defaults();
    puts("adapter profile defaults OK");
    test_steno_host_encoding();
    puts("adapter steno shared host encoding OK");
    test_jis_repeat_and_next_key_mods();
    puts("adapter JIS repeat and next-key modifiers OK");
    test_alt_combo_snapshot_and_shortcut();
    puts("adapter Alt combo snapshot and shortcut OK");
    test_alt_language_switch_and_reset();
    puts("adapter Alt language switch/reset OK");
    test_alt_tap_and_hold_strokes();
    puts("adapter Alt Tap/Hold strokes OK");
    test_jis_sticky_symbol();
    puts("adapter JIS Sticky symbol OK");
    test_real_geometry();
    puts("adapter real geometry OK");
    test_dynamic_combo_tap_hold();
    puts("adapter dynamic combo Tap/Hold OK");
    test_00_uses_normal_zero_hold();
    puts("adapter 00 normal repeat OK");
    test_mode_and_language_clear();
    puts("adapter mode/language reset OK");
    test_steno_entry_ime_on();
    puts("adapter steno entry IME ON OK");
    test_keyboard_language_restored();
    puts("adapter keyboard language/IME restoration OK");
    test_same_key_multiple_owners();
    puts("adapter duplicate key owners OK");
    test_00_with_another_zero_owner();
    puts("adapter 00 with held zero OK");
    test_jis_shifted_symbol();
    puts("adapter JIS literal symbol OK");
    test_steno_number_layer();
    puts("adapter steno numeric layer OK");
    return 0;
}
