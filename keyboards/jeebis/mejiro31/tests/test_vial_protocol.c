// SPDX-License-Identifier: GPL-3.0-or-later
// Run the pinned keyboard's adapter regressions and our real protocol handler.
#include "adapter_fixture.c"
int vial_unlocked;
int vial_unlock_in_progress;
static bool eeprom_valid = true;
bool via_eeprom_is_valid(void) { return eeprom_valid; }
static void request(uint8_t *packet, uint8_t op, uint8_t flags, uint8_t en, uint8_t jp) {
    memset(packet, 0, 32);
    packet[0] = 0xfa; packet[1] = 1; packet[2] = op;
    memcpy(packet + 4, "M31WEB01", 8);
    packet[12] = flags; packet[13] = en; packet[14] = jp;
    raw_hid_receive_kb(packet, 32);
}
static void rule_request(uint8_t *packet, uint8_t op) {
    memset(packet, 0, 32);
    packet[0] = 0xfa; packet[1] = 2; packet[2] = op;
    memcpy(packet + 4, "M31RUL01", 8);
}
static void rule_transfer(uint8_t *packet, const mejiro_rule *rule, uint8_t action) {
    rule_request(packet, 2); raw_hid_receive_kb(packet, 32); assert(packet[3] == 0);
    for (uint8_t i = 0; i < 9; ++i) {
        rule_request(packet, 3);
        packet[12] = i;
        memcpy(packet + 16, (const uint8_t *)rule + i * 16, 16);
        raw_hid_receive_kb(packet, 32);
        assert(packet[3] == 0);
    }
    rule_request(packet, 4); packet[12] = action;
    raw_hid_receive_kb(packet, 32);
    assert(packet[3] == 0);
}
static void test_rule_protocol(void) {
    setup();
    uint8_t packet[32];
    rule_request(packet, 0); raw_hid_receive_kb(packet, 32);
    assert(packet[3] == 0 && packet[12] == 0 && packet[13] == 24 && packet[14] == 1);
    vial_unlocked = 0;
    rule_request(packet, 2); raw_hid_receive_kb(packet, 32);
    assert(packet[3] == 1);
    vial_unlocked = 1;
    mejiro_rule rule = {.kind = 0, .key_len = 3, .value_len = 3};
    memcpy(rule.key, "KA-", 3);
    memcpy(rule.value, "き", 3);
    rule_transfer(packet, &rule, 0);
    rule_request(packet, 0); raw_hid_receive_kb(packet, 32);
    assert(packet[12] == 1);
    mejiro_rule actual = {0};
    for (uint8_t i = 0; i < 9; ++i) {
        rule_request(packet, 1); packet[12] = 0; packet[13] = i;
        raw_hid_receive_kb(packet, 32);
        assert(packet[3] == 0);
        memcpy((uint8_t *)&actual + i * 16, packet + 16, 16);
    }
    assert(!memcmp(&actual, &rule, sizeof(rule)));
    setup(); // Runtime is cleared, then restored from the committed EEPROM bank.
    assert(mejiro_qmk_rule_count() == 1);
    vial_unlocked = 1;
    rule_transfer(packet, &rule, 1);
    assert(mock_rules[0].hidden);
    setup();
    assert(mejiro_qmk_rule_count() == 1 && mock_rules[0].hidden);
    vial_unlocked = 1;
    rule_transfer(packet, &rule, 2);
    setup();
    assert(mejiro_qmk_rule_count() == 0);
    puts("dictionary transport, lock, EEPROM restore, hide and clear OK");
}
static void test_web_protocol(void) {
    setup();
    uint8_t packet[32];
    vial_unlocked = 0;
    request(packet, 0, 0, 0, 0);
    assert(packet[3] == 0 && packet[12] == (2 | (MJI_DEFAULT_MEJIRO ? 4 : 0)) && packet[16] == 14 && packet[17] == WEB_LAYOUT_COUNT && packet[18] == 1);
    uint32_t saved = settings_word;
    request(packet, 2, 0, 0, 0);
    assert(packet[3] == 1 && settings_word == saved && packet[18] == 1);
    request(packet, 1, 7, 1, 16);
    assert(packet[3] == 1 && settings_word == saved);
    vial_unlocked = 1;
    request(packet, 2, 0, 0, 0);
    assert(packet[3] == 0 && packet[18] == 0 && !mejiro_first_up_chord_send);
    assert(settings_word & UINT32_C(0x2000));
    keyboard_post_init_user();
    assert(!mejiro_first_up_chord_send);
    request(packet, 2, 0, 0, 0);
    packet[18] = 1;
    raw_hid_receive_kb(packet, 32);
    assert(packet[3] == 0 && packet[18] == 1 && mejiro_first_up_chord_send);
    assert(!(settings_word & UINT32_C(0x2000)));
    saved = settings_word;
    request(packet, 1, 6, WEB_LAYOUT_COUNT, 0);
    assert(packet[3] == 2 && settings_word == saved);
    // Hold a real key while changing context: no stuck output may survive.
    event(0, 1, true); tick(200);
    request(packet, 1, 7, 1, 16);
    assert(packet[3] == 0 && packet[13] == 1 && packet[14] == 16);
    assert(input.layout.jis_host && input.layout.alt_enabled && mejiro_mode);
    assert(input.ignored & (UINT64_C(1) << 1));
    all_up(); event(201, 1, false);
    keyboard_post_init_user();
    assert(web_en_layout == 1 && web_jp_layout == 16);
    assert(input.layout.maps[0].entries == mji_layout_graphite);
    assert(input.layout.maps[1].entries == mji_layout_o24);
    // Test that a site-selected runtime map actually changes emitted keys.
    event(300, 1, true); tick(500);
    assert(native_keys[KC_B]); // Graphite maps Q -> B.
    event(501, 1, false); all_up();
    packet[0] = 0xfa; packet[1] = 2;
    raw_hid_receive_kb(packet, 32); assert(packet[0] == 0xff);
    setup(); request(packet, 0, 0, 0, 0); // Ready flag is checked before all writes.
    ready = false; request(packet, 1, 6, 0, 0); assert(packet[3] == 3);
    ready = true; request(packet, 0, 0, 0, 0);
    assert(packet[19] == 1 && input.timing.combo_ms == MJI_COMBO_MS);
    packet[2] = 3; packet[20] = 100; packet[21] = 0;
    packet[22] = 40; packet[23] = 0; packet[24] = 250; packet[25] = 0;
    vial_unlocked = 0; raw_hid_receive_kb(packet, 32); assert(packet[3] == 1);
    vial_unlocked = 1; packet[20] = 100; packet[22] = 40; packet[24] = 250; packet[25] = 0;
    raw_hid_receive_kb(packet, 32); assert(packet[3] == 0);
    assert(input.timing.combo_ms == 100 && input.timing.continuation_ms == 40 && input.timing.long_hold_ms == 250);
    assert(web_load_timing().combo_ms == 100 && web_load_timing().long_hold_ms == 250);
    packet[24] = 30; packet[25] = 0;
    raw_hid_receive_kb(packet, 32); assert(packet[3] == 2 && input.timing.long_hold_ms == 250);
    eeprom_valid = false; via_init_kb();
    assert(((settings_word >> 3) & 31) == MJI_WEB_EN_DEFAULT);
    assert(((settings_word >> 8) & 31) == MJI_WEB_JP_DEFAULT);
    puts("site protocol identity, lock, validation, context barrier, persisted layouts and real key output OK");
}
static void test_unlock_barrier(void) {
    setup();
    const uint16_t expected_q = KC_Q; // setup() intentionally selects QWERTY for input regressions.
    event(0, 1, true); tick(200); assert(native_keys[expected_q]);
    vial_unlock_in_progress = 1;
    tick(201); all_up();
    event(202, 0, true);
    event(203, 11, true);
    tick(500); all_up();
    event(501, 1, false);
    event(502, 0, false);
    assert(!(input.physical & 3));
    vial_unlock_in_progress = 0;
    tick(503); all_up();
    assert(input.ignored & (UINT64_C(1) << 11));
    event(504, 11, false); all_up();
    event(600, 1, true); tick(800);
    assert(native_keys[expected_q]);
    event(801, 1, false); all_up();
    puts("unlock barrier suppresses engine output, tracks releases and resumes input OK");
}
static void test_extra_combos(void) {
    const uint8_t pairs[][2] = {{0,12},{0,1},{12,13},{40,41},{22,23},{10,11}};
    for (unsigned i = 0; i < 6; ++i) {
        setup();
        dynamic_maps[12][0][i] = KC_ESC;
        event(0, pairs[i][0], true);
        event(10, pairs[i][1], true);
        tick(250);
        event(251, pairs[i][0], false);
        event(252, pairs[i][1], false);
        tick(500);
        assert(presses[KC_ESC] == 1);
        setup();
        dynamic_maps[13][0][i] = KC_LALT;
        event(0, pairs[i][0], true);
        event(10, pairs[i][1], true);
        tick(500);
        assert(native_mods & MOD_BIT(KC_LALT));
        event(501, pairs[i][0], false);
        event(502, pairs[i][1], false);
        tick(800);
        assert(!(native_mods & MOD_BIT(KC_LALT)));
    }
    setup();
    dynamic_maps[12][0][6] = KC_F1; // Shift Keyboard, combo index 0.
    assert(read_binding_code(1, 48) == KC_F1 && read_binding_code(0, 48) == KC_NO);
    dynamic_maps[13][2][5] = KC_RALT; // Function Hold, combo index 5.
    assert(read_binding_code(10, 53) == KC_RALT);
    dynamic_maps[12][2][6] = KC_ESC;
    mji_context_t steno = {.layers = {MJI_STENO}, .layer_count = 1};
    mji_binding_t inactive = lookup_callback(NULL, 48, &steno);
    assert(inactive.tap == KC_NO && inactive.hold_layer == MJI_NO_LAYER);
    setup();
    event(0, 0, true); event(10, 1, true); tick(250);
    event(251, 0, false); event(252, 1, false); tick(500);
    assert(presses[KC_GRV] == 1 && presses[KC_Q] == 1);
    puts("six extension combos support real Tap/Hold; empty combos preserve normal keys OK");
}
static void test_declarative_defaults(void) {
    setup();
    keyboard_post_init_user();
    assert(web_en_layout == MJI_WEB_EN_DEFAULT && web_jp_layout == MJI_WEB_JP_DEFAULT);
    assert(input.layout.maps[0].entries == web_layouts[MJI_WEB_EN_DEFAULT].entries);
    assert(input.layout.maps[1].entries == web_layouts[MJI_WEB_JP_DEFAULT].entries);
    puts("declarative language/preset defaults preserved in Vial storage OK");
}
int main(void) {
    test_declarative_defaults();
    test_extra_combos();
    assert(adapter_regression_main() == 0);
    test_web_protocol();
    test_rule_protocol();
    test_unlock_barrier();
    return 0;
}
