// SPDX-License-Identifier: GPL-2.0-or-later
#include "../mejiro_layout.h"
#include <assert.h>
#include <stdio.h>

static void expect(mji_stroke_t s, uint16_t code, uint8_t mods, uint8_t clear) {
    assert(s.code == code && s.mods == mods && s.clear_mods == clear);
}
static void test_us_identity(void) {
    mji_layout_config_t config = {0};
    for (uint16_t code = 0; code < 256; code++)
        for (uint8_t i = 0; i < 4; i++) {
            static const uint8_t mods[] = {0, 2, 1, 0x20};
            expect(mji_layout_resolve(&config, (mji_key_t){code, mods[i]}, 0, 0x0f), code, mods[i], 0);
        }
}
static void test_jis_symbols(void) {
    mji_layout_config_t config = {.jis_host = true};
    // Expected physical JIS strokes, independently specified by symbol.
    static const struct {
        uint16_t  us;
        mji_key_t plain, shifted;
    } cases[] = {
        {0x1e, {0x1e, 0}, {0x1e, 2}}, {0x1f, {0x1f, 0}, {0x2f, 0}}, {0x20, {0x20, 0}, {0x20, 2}}, {0x21, {0x21, 0}, {0x21, 2}}, {0x22, {0x22, 0}, {0x22, 2}}, {0x23, {0x23, 0}, {0x2e, 0}}, {0x24, {0x24, 0}, {0x23, 2}}, {0x25, {0x25, 0}, {0x34, 2}}, {0x26, {0x26, 0}, {0x25, 2}}, {0x27, {0x27, 0}, {0x26, 2}}, {0x2d, {0x2d, 0}, {0x87, 2}}, {0x2e, {0x2d, 2}, {0x33, 2}}, {0x2f, {0x30, 0}, {0x30, 2}}, {0x30, {0x32, 0}, {0x32, 2}}, {0x31, {0x87, 0}, {0x89, 2}}, {0x33, {0x33, 0}, {0x34, 0}}, {0x34, {0x24, 2}, {0x1f, 2}}, {0x35, {0x2f, 2}, {0x2e, 2}}, {0x36, {0x36, 0}, {0x36, 2}}, {0x37, {0x37, 0}, {0x37, 2}}, {0x38, {0x38, 0}, {0x38, 2}},
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(*cases); i++) {
        expect(mji_layout_resolve(&config, (mji_key_t){cases[i].us, 0}, 0, 0), cases[i].plain.code, cases[i].plain.mods, 0x22);
        expect(mji_layout_resolve(&config, (mji_key_t){cases[i].us, 0}, 0, 2), cases[i].shifted.code, cases[i].shifted.mods, 0x22);
        expect(mji_layout_resolve(&config, (mji_key_t){cases[i].us, 0x20}, 0, 0), cases[i].shifted.code, cases[i].shifted.mods, 0x22);
    }
    // Ctrl survives translation, including a symbol whose JIS form needs no Shift.
    expect(mji_layout_resolve(&config, (mji_key_t){0x1f, 3}, 0, 0), 0x2f, 1, 0x22);
}
static void test_alt_profiles_and_order(void) {
    static const mji_layout_entry_t english[]  = {{4, {5, 0}, {6, 0}}, {7, {0x2d, 0}, {0x2e, 0}}};
    static const mji_layout_entry_t japanese[] = {{4, {8, 0}, {9, 0}}};
    mji_layout_config_t             c          = {.maps = {{english, 2}, {japanese, 1}}, .alt_layers = 3, .alt_enabled = true};
    expect(mji_layout_resolve(&c, (mji_key_t){4, 0}, 0, 0), 5, 0, 0);
    expect(mji_layout_resolve(&c, (mji_key_t){4, 0}, 0, 2), 6, 0, 0);
    expect(mji_layout_resolve(&c, (mji_key_t){4, 0x20}, 0, 0), 6, 0x20, 0);
    expect(mji_layout_resolve(&c, (mji_key_t){4, 0}, 2, 0), 4, 0, 0);
    for (unsigned bit = 0; bit < 8; bit++)
        if (bit != 1 && bit != 5) {
            expect(mji_layout_resolve(&c, (mji_key_t){4, 0}, 0, 1 << bit), 4, 0, 0);
            expect(mji_layout_resolve(&c, (mji_key_t){4, 1 << bit}, 0, 0), 4, 1 << bit, 0);
        }
    c.language = MJI_JAPANESE;
    expect(mji_layout_resolve(&c, (mji_key_t){4, 0}, 0, 0), 8, 0, 0);
    c.language = MJI_ENGLISH;
    expect(mji_layout_resolve(&c, (mji_key_t){7, 0}, 0, 2), 0x2e, 0, 0x22);
    c.jis_host = true;
    expect(mji_layout_resolve(&c, (mji_key_t){7, 0}, 0, 2), 0x2d, 2, 0x22);
    c.alt_enabled = false;
    expect(mji_layout_resolve(&c, (mji_key_t){4, 0}, 0, 0), 4, 0, 0);
}
int main(void) {
    mji_layout_config_t shortcut = {.jis_host = true};
    expect(mji_layout_resolve(&shortcut, (mji_key_t){0x35, 0}, 0, 0x04), 0x35, 0, 0);
    expect(mji_layout_resolve(&shortcut, (mji_key_t){0x35, 0x40}, 0, 0), 0x35, 0x40, 0);
    test_us_identity();
    puts("layout US identity OK");
    test_jis_symbols();
    puts("layout JIS symbols, left/right Shift, Ctrl OK");
    test_alt_profiles_and_order();
    puts("layout Alt profiles/layers/shortcuts/translation order OK");
}
