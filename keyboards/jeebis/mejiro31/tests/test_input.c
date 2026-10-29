// SPDX-License-Identifier: GPL-2.0-or-later
#include "../mejiro_input.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

enum { E, C, X, R, V, T, B, SPACE, SHIFT, ALT, LAYER, STICKY, OTHER, DUP_SHIFT };
enum { D = 48, F, G, ESC, TAB };
enum { K_E = 100, K_C, K_X, K_D, K_F, K_G, K_ESC, K_TAB, K_SPACE, K_OTHER, K_ALT_C, K_LAYER_C };
#define CTL 1
#define SFT 2
#define ALT_MOD 4
#define PLAIN(code) ((mji_binding_t){.tap = code, .hold_layer = MJI_NO_LAYER})
#define DUAL(code, mod) ((mji_binding_t){.tap = code, .hold_mods = mod, .hold_layer = MJI_NO_LAYER})
#define LAY(code, layer) ((mji_binding_t){.tap = code, .hold_layer = layer})

static const mji_pair_t pairs[] = {
    {E, C, D, 0}, {R, V, F, 0}, {T, B, G, 0}, {R, T, ESC, 1}, {V, B, TAB, 1},
};
typedef struct {
    uint16_t code;
    bool     pressed;
    uint8_t  mods;
} report_t;
typedef struct {
    mji_engine_t  e;
    mji_binding_t map[4][64];
    bool          defined[4][64];
    report_t      log[4096];
    unsigned      count, clears;
    uint8_t       mods, seen_mods;
    bool          held[512];
    uint32_t      clock;
} fixture_t;
static fixture_t     f;
static mji_binding_t lookup(void *user, uint16_t logical, const mji_context_t *c) {
    fixture_t *s = user;
    assert(logical < 64);
    for (uint8_t i = 0; i < c->layer_count; ++i) {
        uint8_t layer = c->layers[i];
        assert(layer < 4);
        if (s->defined[layer][logical]) return s->map[layer][logical];
    }
    return PLAIN(0);
}
static void key(void *user, uint8_t source, uint16_t code, bool pressed, uint8_t mods) {
    (void)source;
    fixture_t *s = user;
    assert(s->count < sizeof(s->log) / sizeof(s->log[0]));
    assert(code < 512);
    s->log[s->count++] = (report_t){code, pressed, mods};
    s->held[code]      = pressed;
}
static void mods(void *user, uint8_t value) {
    ((fixture_t *)user)->mods = value;
    ((fixture_t *)user)->seen_mods |= value;
}
static void clear(void *user) {
    fixture_t *s = user;
    memset(s->held, 0, sizeof(s->held));
    s->mods = 0;
    s->clears++;
}
static void bind(uint8_t layer, uint8_t logical, mji_binding_t b) {
    f.defined[layer][logical] = true;
    f.map[layer][logical]     = b;
}
static void setup(void) {
    memset(&f, 0, sizeof(f));
    mji_callbacks_t cb = {.lookup = lookup, .key = key, .mods = mods, .clear = clear};
    assert(mji_init(&f.e, pairs, sizeof(pairs) / sizeof(*pairs), (mji_timing_t){40, 15, 200}, 0, cb, &f));
    bind(0, E, PLAIN(K_E));
    bind(0, C, PLAIN(K_C));
    bind(0, X, PLAIN(K_X));
    bind(0, D, PLAIN(K_D));
    bind(0, F, PLAIN(K_F));
    bind(0, G, PLAIN(K_G));
    bind(0, ESC, PLAIN(K_ESC));
    bind(0, TAB, PLAIN(K_TAB));
    bind(0, R, PLAIN('r'));
    bind(0, T, PLAIN('t'));
    bind(0, V, PLAIN('v'));
    bind(0, B, PLAIN('b'));
    bind(0, SPACE, DUAL(K_SPACE, SFT));
    bind(0, SHIFT, DUAL(0, SFT));
    bind(0, DUP_SHIFT, DUAL(0, SFT));
    bind(0, ALT, DUAL(0, ALT_MOD));
    bind(0, LAYER, LAY(0, 1));
    bind(0, STICKY, PLAIN(MJI_STICKY_SHIFT));
    bind(0, OTHER, PLAIN(K_OTHER));
}
static void at(uint32_t time, uint8_t p, bool down) {
    f.clock = time;
    mji_event(&f.e, p, down, time);
}
static void tick(uint32_t time) {
    f.clock = time;
    mji_tick(&f.e, time);
}
static void expect(unsigned i, uint16_t code, bool down, uint8_t mod) {
    if (i >= f.count || f.log[i].code != code || f.log[i].pressed != down || f.log[i].mods != mod) {
        fprintf(stderr, "report %u expected %u/%d/%u; count=%u", i, code, down, mod, f.count);
        if (i < f.count) fprintf(stderr, " actual=%u/%d/%u", f.log[i].code, f.log[i].pressed, f.log[i].mods);
        fputc('\n', stderr);
        assert(false);
    }
}
static void no_stuck(void) {
    assert(!f.e.physical);
    assert(!f.mods);
    for (unsigned i = 0; i < 512; i++)
        assert(!f.held[i]);
    assert(!f.e.event_count);
}
static void test_interleaved(void) {
    setup();
    at(0, E, true);
    at(5, X, true);
    assert(!f.count);
    at(10, C, true);
    expect(0, K_D, true, 0);
    expect(1, K_X, true, 0);
    at(20, E, false);
    at(21, C, false);
    at(22, X, false);
    no_stuck();
}
static void test_late_shift(void) {
    setup();
    at(0, E, true);
    at(5, SHIFT, true);
    at(10, C, true);
    expect(0, K_D, true, 0);
    assert(f.mods == SFT);
    at(20, E, false);
    at(21, C, false);
    at(22, SHIFT, false);
    no_stuck();
}
static void test_released_shift_while_pending(void) {
    setup();
    at(0, SPACE, true);
    at(5, E, true);
    at(10, SPACE, false);
    assert(!f.count);
    at(15, C, true);
    expect(0, K_D, true, SFT);
    assert(f.mods == 0);
    assert(f.count == 1);
    at(20, E, false);
    at(21, C, false);
    no_stuck();
}
static void test_multi_vertical_priority(void) {
    setup();
    at(0, R, true);
    at(5, T, true);
    assert(!f.count);
    at(10, V, true);
    at(15, B, true);
    expect(0, K_F, true, 0);
    expect(1, K_G, true, 0);
    assert(f.held[K_F] && f.held[K_G]);
    at(20, R, false);
    at(21, V, false);
    assert(f.held[K_G]);
    at(22, T, false);
    at(23, B, false);
    no_stuck();
}
static void test_horizontal_timeout(void) {
    setup();
    at(0, R, true);
    at(5, T, true);
    tick(39);
    assert(!f.count);
    tick(40);
    expect(0, K_ESC, true, 0);
    at(45, R, false);
    expect(1, K_ESC, false, 0);
    at(46, T, false);
    no_stuck();
}
static void test_horizontal_release(void) {
    setup();
    at(0, R, true);
    at(5, T, true);
    at(10, R, false);
    expect(0, K_ESC, true, 0);
    expect(1, K_ESC, false, 0);
    at(11, T, false);
    assert(f.count == 2);
    no_stuck();
}
static void test_confirmed_horizontal(void) {
    setup();
    at(0, V, true);
    tick(40);
    expect(0, 'v', true, 0);
    at(50, B, true);
    expect(1, 'v', false, 0);
    expect(2, K_TAB, true, 0);
    at(55, R, true);
    tick(95);
    expect(3, 'r', true, 0);
    assert(f.held[K_TAB]);
    at(100, V, false);
    at(101, B, false);
    at(102, R, false);
    no_stuck();
}
static void test_switch_new_shift(void) {
    setup();
    at(0, C, true);
    tick(40);
    at(50, SHIFT, true);
    at(60, E, true);
    expect(0, K_C, true, 0);
    expect(1, K_C, false, SFT);
    expect(2, K_D, true, SFT);
    at(70, C, false);
    at(71, E, false);
    at(72, SHIFT, false);
    no_stuck();
}
static void test_release_continuation(void) {
    setup();
    at(0, E, true);
    at(5, C, true);
    at(10, E, false);
    assert(f.count == 2);
    tick(24);
    assert(f.count == 2);
    tick(25);
    expect(2, K_C, true, 0);
    at(30, C, false);
    expect(3, K_C, false, 0);
    no_stuck();
}
static void test_repress_pair(void) {
    setup();
    at(0, E, true);
    at(5, C, true);
    at(10, E, false);
    at(15, E, true);
    expect(2, K_D, true, 0);
    assert(f.count == 3);
    at(20, E, false);
    at(21, C, false);
    no_stuck();
}
static void test_repress_single(void) {
    setup();
    at(0, E, true);
    at(5, E, false);
    at(10, E, true);
    at(15, C, true);
    expect(0, K_E, true, 0);
    expect(1, K_E, false, 0);
    expect(2, K_D, true, 0);
    at(20, E, false);
    at(21, C, false);
    no_stuck();
}
static void test_dual_tap(void) {
    setup();
    at(0, SPACE, true);
    assert(!f.mods && !f.seen_mods);
    at(20, SPACE, false);
    expect(0, K_SPACE, true, 0);
    expect(1, K_SPACE, false, 0);
    no_stuck();
}
static void test_quick_pending_tap_without_mods(void) {
    setup();
    bind(0, C, DUAL(K_C, CTL));
    at(0, C, true);
    assert(!f.seen_mods);
    at(5, C, false);
    expect(0, K_C, true, 0);
    expect(1, K_C, false, 0);
    assert(f.count == 2 && !f.seen_mods);
    no_stuck();

    setup();
    bind(0, C, DUAL(K_C, CTL));
    at(0, C, true);
    tick(40);
    assert(f.mods == CTL && f.seen_mods == CTL);
    at(45, C, false);
    expect(0, K_C, true, 0);
    no_stuck();
}
static void test_dual_long(void) {
    setup();
    at(0, SPACE, true);
    tick(39);
    assert(!f.mods && !f.seen_mods);
    tick(40);
    assert(f.mods == SFT);
    tick(200);
    at(210, SPACE, false);
    assert(!f.count);
    no_stuck();
}
static void test_roll_is_hold(void) {
    setup();
    bind(0, C, DUAL(K_C, CTL));
    at(0, C, true);
    at(5, X, true);
    at(10, C, false);
    expect(0, K_X, true, CTL);
    assert(f.count == 1 && !f.mods);
    at(15, X, false);
    no_stuck();
}
static void test_combined_mods(void) {
    setup();
    bind(0, OTHER, DUAL(K_OTHER, CTL));
    at(0, SHIFT, true);
    at(5, OTHER, true);
    at(10, X, true);
    expect(0, K_X, true, CTL | SFT);
    at(15, SHIFT, false);
    assert(f.mods == CTL);
    at(20, OTHER, false);
    at(25, X, false);
    no_stuck();
}
static void test_own_hold_not_in_tap(void) {
    setup();
    bind(0, OTHER, DUAL(K_SPACE, CTL));
    at(0, SHIFT, true);
    at(5, OTHER, true);
    at(10, OTHER, false);
    expect(0, K_SPACE, true, SFT);
    expect(1, K_SPACE, false, SFT);
    assert(f.mods == SFT);
    at(15, SHIFT, false);
    no_stuck();
}
static void test_duplicate_mod_sources(void) {
    setup();
    at(0, SHIFT, true);
    at(5, DUP_SHIFT, true);
    at(10, SHIFT, false);
    assert(f.mods == SFT);
    at(15, DUP_SHIFT, false);
    no_stuck();
}
static void test_layers_and_transparency(void) {
    setup();
    bind(1, OTHER, LAY(0, 2));
    bind(1, C, DUAL(K_ALT_C, CTL));
    bind(2, C, PLAIN(K_LAYER_C));
    at(0, LAYER, true);
    at(5, OTHER, true);
    at(10, C, true);
    tick(50);
    expect(0, K_LAYER_C, true, 0);
    at(55, LAYER, false);
    at(60, OTHER, false);
    assert(f.held[K_LAYER_C]);
    at(65, C, false);
    expect(1, K_LAYER_C, false, 0);
    no_stuck();
}
static void test_layer_lookup_frozen(void) {
    setup();
    bind(1, D, PLAIN(K_LAYER_C));
    at(0, E, true);
    at(5, LAYER, true);
    at(10, C, true);
    expect(0, K_D, true, 0);
    at(15, E, false);
    at(16, C, false);
    at(17, LAYER, false);
    no_stuck();
}
static void test_sticky_target(void) {
    setup();
    at(0, STICKY, true);
    at(1, STICKY, false);
    assert(f.e.sticky_waiting);
    at(5, E, true);
    at(10, C, true);
    expect(0, K_D, true, SFT);
    assert(f.mods == SFT);
    at(15, E, false);
    assert(!f.mods);
    at(16, C, false);
    no_stuck();
}
static void test_sticky_next_key(void) {
    setup();
    at(0, STICKY, true);
    at(1, STICKY, false);
    at(5, C, true);
    tick(45);
    expect(0, K_C, true, SFT);
    at(50, X, true);
    expect(1, K_X, true, 0);
    assert(!f.mods);
    at(55, C, false);
    at(56, X, false);
    no_stuck();
}
static void test_sticky_toggle(void) {
    setup();
    at(0, STICKY, true);
    at(1, STICKY, false);
    at(2, STICKY, true);
    at(3, STICKY, false);
    assert(!f.e.sticky_waiting);
    at(5, X, true);
    expect(0, K_X, true, 0);
    at(6, X, false);
    no_stuck();
}
static void test_sticky_with_ctrl_hold(void) {
    setup();
    bind(0, SPACE, DUAL(K_SPACE, CTL));
    at(0, STICKY, true);
    at(1, STICKY, false);
    at(5, SPACE, true);
    at(10, C, true);
    tick(50);
    expect(0, K_C, true, CTL | SFT);
    at(55, SPACE, false);
    assert(f.mods == SFT);
    at(60, C, false);
    no_stuck();
}
static void test_reset_suppression(void) {
    setup();
    at(0, SPACE, true);
    at(1, C, true);
    at(2, STICKY, true);
    mji_reset(&f.e, 0);
    assert(f.clears == 1 && !f.mods && !f.e.sticky_waiting);
    tick(300);
    assert(!f.count);
    at(310, SPACE, false);
    at(311, C, false);
    at(312, STICKY, false);
    at(320, X, true);
    expect(0, K_X, true, 0);
    at(321, X, false);
    no_stuck();
}
static void test_residual_timer_origin(void) {
    setup();
    bind(0, C, DUAL(K_C, CTL));
    at(0, E, true);
    at(5, C, true);
    at(10, E, false);
    tick(25);
    assert(f.mods == CTL);
    at(209, C, false);
    expect(2, K_C, true, 0);
    no_stuck();
    setup();
    bind(0, C, DUAL(K_C, CTL));
    at(0, E, true);
    at(5, C, true);
    at(10, E, false);
    tick(25);
    at(210, C, false);
    assert(f.count == 2);
    no_stuck();
}
static void test_residual_does_not_block(void) {
    setup();
    bind(0, C, DUAL(K_C, CTL));
    at(0, E, true);
    at(5, C, true);
    at(10, E, false);
    at(15, X, true);
    expect(2, K_X, true, 0);
    tick(25);
    assert(f.mods == CTL);
    at(30, C, false);
    at(31, X, false);
    no_stuck();
}
static void test_residual_affects_unresolved(void) {
    setup();
    bind(0, C, DUAL(K_C, CTL));
    at(0, E, true);
    at(5, C, true);
    at(10, E, false);
    at(15, R, true);
    tick(25);
    tick(55);
    expect(2, 'r', true, CTL);
    at(60, C, false);
    assert(f.count == 3);
    at(61, R, false);
    no_stuck();
}
static void test_switch_clears_own_hold(void) {
    setup();
    bind(0, V, DUAL('v', CTL));
    at(0, V, true);
    tick(250);
    assert(f.mods == CTL);
    at(260, B, true);
    expect(0, K_TAB, true, 0);
    assert(f.count == 1);
    at(270, V, false);
    at(271, B, false);
    no_stuck();
}
static void test_clock_wrap(void) {
    setup();
    at(UINT32_MAX - 20, E, true);
    at(UINT32_MAX - 10, C, true);
    expect(0, K_D, true, 0);
    at(0, E, false);
    at(1, C, false);
    no_stuck();
    setup();
    at(UINT32_MAX - 20, C, true);
    tick(20);
    expect(0, K_C, true, 0);
    at(25, C, false);
    no_stuck();
}
static void test_repeated_key_no_slot_leak(void) {
    setup();
    for (unsigned i = 0; i < 1000; i++) {
        at(i * 4, X, true);
        at(i * 4 + 1, X, false);
    }
    assert(f.count == 2000 && f.e.overflows == 0);
    no_stuck();
}
static void test_invalid_and_duplicate_events(void) {
    setup();
    at(0, 255, true);
    at(0, X, false);
    at(1, X, true);
    at(2, X, true);
    at(3, X, false);
    assert(f.count == 2);
    no_stuck();
    mji_timing_t bad = {40, 20, 20};
    assert(!mji_init(&f.e, pairs, 5, bad, 0, f.e.callbacks, &f));
}
static void test_many_keys_held(void) {
    setup();
    for (unsigned i = 14; i < 31; i++) {
        bind(0, i, PLAIN(200 + i));
        at(i, i, true);
    }
    for (unsigned i = 14; i < 31; i++)
        assert(f.held[200 + i]);
    for (unsigned i = 14; i < 31; i++)
        at(100 + i, i, false);
    no_stuck();
}
static void test_random_release_balance(void) {
    setup();
    bind(0, C, DUAL(K_C, CTL));
    bind(0, D, DUAL(K_D, SFT));
    uint32_t rng = 0x912c31, time = 0;
    for (unsigned i = 0; i < 50000; i++) {
        rng       = rng * 1664525U + 1013904223U;
        uint8_t p = (rng >> 16) % 14;
        time += (rng >> 24) % 7;
        bool down = !(f.e.physical & (UINT64_C(1) << p));
        at(time, p, down);
        f.count = 0;
        if (i % 100 == 99) {
            for (uint8_t k = 0; k < 14; k++)
                if (f.e.physical & (UINT64_C(1) << k)) at(++time, k, false);
            tick(time += 250);
            no_stuck();
        }
    }
    assert(!f.e.overflows);
}
static void test_plain_release_not_blocked_by_pending_press(void) {
    setup();
    f.e.timing = (mji_timing_t){150, 50, 300}; // Firmware timings.
    at(0, OTHER, true);
    at(160, E, true); // This start cannot resolve until 310ms.
    at(170, OTHER, false);
    expect(0, K_OTHER, true, 0);
    expect(1, K_OTHER, false, 0);
    assert(!f.held[K_OTHER]);
    tick(250); // The host must not still see OTHER at its repeat deadline.
    assert(f.count == 2);
    tick(310);
    expect(2, K_E, true, 0);
    at(320, E, false);
    expect(3, K_E, false, 0);
    no_stuck();
}
static void test_residual_output_preserves_order(void) {
    setup();
    at(0, E, true);
    at(5, C, true);
    at(8, R, true);
    at(10, E, false);
    tick(25);
    assert(f.count == 2); // D releases now; R/C starts retain their order.
    expect(1, K_D, false, 0);
    tick(48);
    expect(2, 'r', true, 0);
    expect(3, K_C, true, 0);
    at(50, C, false);
    at(51, R, false);
    no_stuck();
}
static void test_released_residual_not_used_by_later_confirmation(void) {
    setup();
    bind(0, C, DUAL(K_C, CTL));
    at(0, E, true);
    at(5, C, true);
    at(8, R, true);
    at(10, E, false);
    tick(25);
    at(30, C, false);
    tick(48);
    expect(1, K_D, false, 0);
    expect(2, 'r', true, 0);
    at(50, R, false);
    no_stuck();
}
static void test_capacity_fails_closed(void) {
    setup();
    at(0, R, true);
    for (unsigned i = 0; i < 110; i++) {
        at(1, X, true);
        at(1, X, false);
        f.count = 0;
    }
    assert(f.e.overflows > 0);
    assert(f.clears > 0);
    assert(!f.mods);
    at(2, R, false);
    tick(300);
    no_stuck();
}
static void test_equal_timestamps_preserve_event_order(void) {
    setup();
    at(1, E, true);
    at(1, X, true);
    at(1, C, true);
    expect(0, K_D, true, 0);
    expect(1, K_X, true, 0);
    at(2, E, false);
    at(2, C, false);
    at(2, X, false);
    no_stuck();
    setup();
    at(1, C, true);
    at(1, X, true);
    at(1, E, true);
    expect(0, K_D, true, 0);
    expect(1, K_X, true, 0);
    at(2, E, false);
    at(2, C, false);
    at(2, X, false);
    no_stuck();
    setup();
    at(1, R, true);
    at(1, T, true);
    at(1, V, true);
    at(1, B, true);
    expect(0, K_F, true, 0);
    expect(1, K_G, true, 0);
    at(2, R, false);
    at(2, T, false);
    at(2, V, false);
    at(2, B, false);
    no_stuck();
}
static void test_combo_long_hold_origin(void) {
    setup();
    bind(0, D, DUAL(K_D, CTL));
    at(0, E, true);
    at(30, C, true);
    at(199, E, false);
    expect(0, K_D, true, 0);
    expect(1, K_D, false, 0);
    at(200, C, false);
    no_stuck();
    setup();
    bind(0, D, DUAL(K_D, CTL));
    at(0, E, true);
    at(30, C, true);
    at(200, E, false);
    assert(!f.count);
    at(201, C, false);
    no_stuck();
}
int main(void) {
#define RUN(test)          \
    do {                   \
        test();            \
        puts(#test " OK"); \
    } while (0)
    RUN(test_interleaved);
    RUN(test_late_shift);
    RUN(test_released_shift_while_pending);
    RUN(test_multi_vertical_priority);
    RUN(test_horizontal_timeout);
    RUN(test_horizontal_release);
    RUN(test_confirmed_horizontal);
    RUN(test_switch_new_shift);
    RUN(test_release_continuation);
    RUN(test_repress_pair);
    RUN(test_repress_single);
    RUN(test_dual_tap);
    RUN(test_quick_pending_tap_without_mods);
    RUN(test_dual_long);
    RUN(test_roll_is_hold);
    RUN(test_combined_mods);
    RUN(test_own_hold_not_in_tap);
    RUN(test_duplicate_mod_sources);
    RUN(test_layers_and_transparency);
    RUN(test_layer_lookup_frozen);
    RUN(test_sticky_target);
    RUN(test_sticky_next_key);
    RUN(test_sticky_toggle);
    RUN(test_sticky_with_ctrl_hold);
    RUN(test_reset_suppression);
    RUN(test_residual_timer_origin);
    RUN(test_residual_does_not_block);
    RUN(test_residual_affects_unresolved);
    RUN(test_switch_clears_own_hold);
    RUN(test_clock_wrap);
    RUN(test_repeated_key_no_slot_leak);
    RUN(test_invalid_and_duplicate_events);
    RUN(test_many_keys_held);
    RUN(test_random_release_balance);
    RUN(test_plain_release_not_blocked_by_pending_press);
    RUN(test_residual_output_preserves_order);
    RUN(test_released_residual_not_used_by_later_confirmation);
    RUN(test_capacity_fails_closed);
    RUN(test_equal_timestamps_preserve_event_order);
    RUN(test_combo_long_hold_origin);
}
