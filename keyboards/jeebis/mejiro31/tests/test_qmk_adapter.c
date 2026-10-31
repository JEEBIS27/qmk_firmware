// SPDX-License-Identifier: GPL-2.0-or-later
#include <assert.h>
#include <stdio.h>
#include "report.h"
#include "keymap_japanese.h"
#ifdef MJI_TEST_EN
#    include "../keymaps/en_default/keymap.c"
#else
#    include "../keymaps/jp_default/keymap.c"
#endif
static uint8_t pressed_mods[256];

struct stub_keymap_config keymap_config;
static uint8_t            native_mods, reported_mods;
static bool               native_keys[256];
static uint32_t           now;
static unsigned           presses[256], steno_events;
static unsigned           steno_packets;
static int                steno_pending;
static uint16_t           dynamic_maps[14][4][12];
static uint32_t           settings_word;
static uint32_t timing_word;
static unsigned           settings_writes;
static uint16_t consumer_usage, system_usage;
bool mejiro_first_up_chord_send = true;
#ifdef MJI_WEB_VIAL
static uint8_t mock_eeprom[12288];
static mejiro_rule mock_rules[MEJIRO_RULE_CAPACITY];
static int32_t mock_rule_count;
void eeprom_read_block(void *destination, const void *address, size_t size) {
    size_t offset = (uintptr_t)address;
    assert(offset + size <= sizeof(mock_eeprom));
    memcpy(destination, mock_eeprom + offset, size);
}
void eeprom_update_block(const void *source, void *address, size_t size) {
    size_t offset = (uintptr_t)address;
    assert(offset + size <= sizeof(mock_eeprom));
    memcpy(mock_eeprom + offset, source, size);
}
int32_t mejiro_qmk_rule_clear_all(void) { mock_rule_count = 0; return 0; }
int32_t mejiro_qmk_rule_count(void) { return mock_rule_count; }
int32_t mejiro_qmk_rule_get(size_t index, mejiro_rule *output) {
    if (index >= (size_t)mock_rule_count) return -3;
    *output = mock_rules[index];
    return 0;
}
static int mock_find(uint8_t kind, const uint8_t *key, size_t len) {
    for (int i = 0; i < mock_rule_count; ++i)
        if (mock_rules[i].kind == kind && mock_rules[i].key_len == len && !memcmp(mock_rules[i].key, key, len)) return i;
    return -1;
}
int32_t mejiro_qmk_rule_set(const mejiro_rule *rule) {
    if (rule->kind > 4 || rule->key_len > MEJIRO_RULE_KEY_CAPACITY ||
        rule->value_len > MEJIRO_RULE_VALUE_CAPACITY || rule->row_len > MEJIRO_RULE_ROW_CAPACITY ||
        (rule->kind != 4 && rule->row_len)) return -1;
    int i = mock_find(rule->kind, rule->key, rule->key_len);
    if (i < 0) {
        if (mock_rule_count == MEJIRO_RULE_CAPACITY) return -2;
        i = mock_rule_count++;
    }
    mock_rules[i] = *rule;
    return 0;
}
int32_t mejiro_qmk_rule_hide(uint8_t kind, const uint8_t *key, size_t len) {
    mejiro_rule rule = {.kind = kind, .hidden = 1, .key_len = (uint8_t)len};
    if (len > MEJIRO_RULE_KEY_CAPACITY) return -1;
    memcpy(rule.key, key, len);
    int i = mock_find(kind, key, len);
    if (i < 0) {
        if (mock_rule_count == MEJIRO_RULE_CAPACITY) return -2;
        i = mock_rule_count++;
    }
    mock_rules[i] = rule;
    return 0;
}
int32_t mejiro_qmk_rule_clear(uint8_t kind, const uint8_t *key, size_t len) {
    int i = mock_find(kind, key, len);
    if (i < 0) return 0;
    memmove(mock_rules + i, mock_rules + i + 1, (size_t)(mock_rule_count - i - 1) * sizeof(*mock_rules));
    --mock_rule_count;
    return 0;
}
#endif
void host_consumer_send(uint16_t usage) { consumer_usage = usage; }
void host_system_send(uint16_t usage) { system_usage = usage; }
uint32_t eeconfig_read_kb(void) { return timing_word; }
void eeconfig_update_kb(uint32_t value) { timing_word = value; }
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
void send_keyboard_report(void) { reported_mods |= native_mods; }
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
static uint8_t plover_state[8];
static unsigned plover_reports;
void host_plover_hid_send(report_plover_hid_t *report) {
    assert(report->report_id == 0x50);
    int down = 0;
    for (unsigned i = 0; i < 8; ++i) {
        steno_events += __builtin_popcount(plover_state[i] ^ report->data[i]);
        down += __builtin_popcount(report->data[i]);
    }
    if (steno_pending && !down) ++steno_packets;
    steno_pending = down;
    memcpy(plover_state, report->data, 8);
    ++plover_reports;
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
    assert(l < 14 && r < 4 && c < 12);
    return dynamic_maps[l][r][c];
}
static void setup(void) {
#ifdef MJI_WEB_VIAL
    mock_rule_count = 0;
#endif
    mejiro_plover_reset();
    memset(plover_state, 0, sizeof(plover_state));
    plover_reports = 0;
    settings_word = UINT32_MAX;
    settings_writes = 0;
    memset(presses, 0, sizeof(presses));
    clear_keyboard();
    reported_mods = 0;
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
    shortcut_active = false;
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
    assert(!steno_pending);
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
    assert(!native_mods);
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
static void test_plover_transport_routes(void) {
    setup();
    mejiro_mode = false;
    steno_mode = true;
    switch_context();
    dynamic_maps[5][0][1] = STN_S1;
    dynamic_maps[5][1][1] = STN_S1;
    event(0, 1, true);
    event(1, 13, true);
    assert(plover_reports == 1 && plover_state[0] == 0x80);
    event(2, 1, false);
    assert(plover_reports == 1 && steno_pending == 1);
    event(3, 13, false);
    assert(plover_reports == 2 && steno_packets == 1);
    all_up();

    event(4, 1, true);
    steno_mode = false;
    switch_context();
    assert(plover_reports == 4 && !steno_pending && steno_packets == 2);
    event(5, 1, false);
    assert(plover_reports == 4); // release remains on its original route
    all_up();

    setup();
    steno_mode = true;
    mejiro_mode = true;
    switch_context();
    event(0, 1, true);
    event(1, 1, false);
    assert(!plover_reports); // onboard Mejiro never leaks a Plover stroke
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
            event(0, 1, true); // Onboard chord cancels; already-sent HID chord closes on reset.
            event(5, 40, true);
            assert(shortcut_active && !steno_held && !steno_pending);
            assert(steno_packets == (mode ? 0U : 1U));
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
            if (!mode) assert(steno_packets == 2 && !steno_pending);
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
// Exercise the default Shift faces through physical input and host encoding.
static void test_shift_face_literal_symbols(void) {
    const uint8_t positions[] = {26, 24, 3, 15, 2, 14, 27};
    const uint16_t codes[] = {KC_EQL, KC_SLSH, KC_LBRC, KC_RBRC, KC_LCBR, KC_RCBR, KC_PLUS};
    for (unsigned jis = 0; jis < 2; ++jis) {
        for (unsigned i = 0; i < sizeof(positions); ++i) {
            setup();
            mji_layout_config_t config = input.layout;
            config.jis_host = jis;
            assert(select_layout(&config));
            event(0, 29, true); // Number
            event(1, 28, true); // Shift
            event(2, positions[i], true);
            tick(100);
            mji_key_t key;
            assert(decode_callback(NULL, codes[i], &key));
            mji_stroke_t expected = mji_layout_resolve(&config, key, MJI_NO_LAYER, 0);
            assert(native_keys[expected.code]);
            assert((pressed_mods[expected.code] & MOD_MASK_SHIFT) == (expected.mods & MOD_MASK_SHIFT));
            event(110, positions[i], false);
            event(111, 28, false);
            event(112, 29, false);
            all_up();
        }
        setup();
        mji_layout_config_t config = input.layout;
        config.jis_host = jis;
        config.maps[0] = config.maps[1] = (mji_layout_map_t)MJI_LAYOUT_MAP(mji_layout_graphite);
        assert(select_layout(&config));
        event(0, 28, true);
        event(1, 22, true); // Main-finger / position: Graphite Shift is <.
        tick(100);
        assert(native_keys[KC_COMM]);
        assert(pressed_mods[KC_COMM] & MOD_MASK_SHIFT);
        event(110, 22, false);
        event(111, 28, false);
        all_up();

        setup();
        config = input.layout;
        config.jis_host = jis;
        config.maps[0] = config.maps[1] = (mji_layout_map_t)MJI_LAYOUT_MAP(mji_layout_graphite);
        assert(select_layout(&config));
        dynamic_maps[1][3][7] = KC_QUES;
        dynamic_maps[7][3][7] = KC_NO;
        event(0, 28, true);
        event(1, 43, true); // Thumb ? must not become Graphite <.
        tick(100);
        assert(native_keys[KC_SLSH]);
        assert(pressed_mods[KC_SLSH] & MOD_MASK_SHIFT);
        event(110, 43, false);
        event(111, 28, false);
        all_up();

        mji_context_t context = {.layers = {MJI_QWERTY}, .layer_count = 1};
        for (uint16_t logical = 0; logical < MJI_PHYSICAL_COUNT; ++logical) {
            bool thumb = logical == 28 || logical == 29 || logical == 30 ||
                         logical == 40 || logical == 41 || logical == 42 ||
                         logical == 43 || logical == 46;
            assert(lookup_callback(NULL, logical, &context).skip_alt == thumb);
        }
    }
}
// Right-thumb combo: literal @/# on Tap; pure Ctrl+Alt on Hold.
static void test_thumb_symbol_ctrl_alt(void) {
    for (unsigned jis = 0; jis < 2; ++jis) {
        for (uint8_t layer = MJI_QWERTY; layer <= MJI_NUMBER_SHIFTED; ++layer) {
            for (unsigned hold = 0; hold < 2; ++hold) {
                setup();
                mji_layout_config_t config = input.layout;
                config.jis_host = jis;
                config.maps[0] = config.maps[1] = (mji_layout_map_t)MJI_LAYOUT_MAP(mji_layout_graphite);
                assert(select_layout(&config));
                mji_reset(&input, layer & ~1);
                if (layer & 1) event(0, 28, true); // Real held Shift selects the face.
                event(1, 42, true);
                event(2, 43, true);
                uint16_t symbol = layer == MJI_QWERTY ? KC_AT : KC_HASH;
                mji_key_t key;
                assert(decode_callback(NULL, symbol, &key));
                mji_stroke_t expected = mji_layout_resolve(&config, key, MJI_NO_LAYER, 0);
                if (hold) {
                    tick(300);
                    assert((native_mods & ~MOD_MASK_SHIFT) == (MOD_LCTL | MOD_LALT));
                    assert(!presses[expected.code]);
                    event(301, 9, true);
                    tick(350);
                    uint8_t next = layer < MJI_NUMBER ? KC_O : KC_END;
                    assert(native_keys[next]);
                    assert((pressed_mods[next] & ~MOD_MASK_SHIFT) == (MOD_LCTL | MOD_LALT));
                    event(351, 9, false);
                }
                event(hold ? 400 : 10, 42, false);
                event(hold ? 401 : 11, 43, false);
                if (layer & 1) event(hold ? 402 : 12, 28, false);
                if (!hold) {
                    assert(presses[expected.code] == 1);
                    assert(pressed_mods[expected.code] == expected.mods);
                }
                all_up();
            }
        }
    }
}
static void test_release_before_pending_combo_timeout(void) {
    setup();
    input.timing = (mji_timing_t){150, 50, 300};
    dynamic_maps[0][0][0] = KC_A;
    event(0, 0, true);
    assert(native_keys[KC_A]);
    event(160, 1, true); // Q remains a combo candidate until 310ms.
    event(170, 0, false);
    assert(!native_keys[KC_A]);
    tick(250);
    assert(!native_keys[KC_A] && presses[KC_A] == 1);
    tick(310);
    assert(native_keys[KC_Q]);
    event(320, 1, false);
    all_up();
}
static void test_space_enter_share_shift(void) {
    for (uint8_t layer = MJI_QWERTY; layer <= MJI_FUNCTION; ++layer) {
        assert(keymaps[layer + MJI_HOLD_OFFSET][2][4] == KC_LSFT);
        assert(keymaps[layer + MJI_HOLD_OFFSET][2][6] == KC_LSFT);
        for (unsigned space_first = 0; space_first < 2; ++space_first) {
            setup();
            mji_reset(&input, layer);
            event(0, 28, true);
            assert(!native_mods);
            event(1, 30, true);
            // Never emit Left+Right Shift: that can activate host Caps Lock.
            assert(native_mods == MOD_LSFT);
            event(10, space_first ? 28 : 30, false);
            assert(native_mods == MOD_LSFT);
            if (!space_first) {
                assert(presses[KC_ENT] == 1);
                assert(pressed_mods[KC_ENT] == MOD_LSFT);
            }
            event(11, space_first ? 30 : 28, false);
            assert(presses[KC_ENT] == 1 && presses[KC_SPC] == 0);
            all_up();
            event(20, 28, true);
            tick(400); // A subsequent long Space also releases cleanly.
            event(401, 28, false);
            all_up();
        }
    }
}
static void test_quick_thumb_mod_suppression(void) {
    const uint8_t positions[] = {28, 30, 46};
    for (unsigned i = 0; i < sizeof(positions); ++i) {
        setup();
        input.timing = (mji_timing_t){150, 50, 300};
        uint8_t p = positions[i];
        if (p == 46) {
            event(0, 42, true);
            event(1, 43, true);
            event(20, 42, false);
            event(21, 43, false);
        } else {
            event(0, p, true);
            assert(!native_mods);
            event(20, p, false);
        }
        // US @ needs Shift for its Tap; Ctrl/Alt must never leak from Hold.
        assert(!(reported_mods & (MOD_MASK_CTRL | MOD_MASK_ALT)));
        if (p != 46) assert(!reported_mods);
        assert(presses[p == 28 ? KC_SPC : p == 30 ? KC_ENT :
                       input.layout.jis_host ? JP_AT : KC_2] == 1);
        all_up();
    }
}
int main(void) {
    test_quick_thumb_mod_suppression();
    test_space_enter_share_shift();
    test_release_before_pending_combo_timeout();
    test_thumb_symbol_ctrl_alt();
    test_shift_face_literal_symbols();
    test_plover_transport_routes();
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
