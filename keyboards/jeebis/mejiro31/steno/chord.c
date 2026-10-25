// SPDX-License-Identifier: GPL-3.0-or-later
// Capture a physical steno chord; all language conversion lives in Rust.
#include "chord.h"
#include "engine.h"

#include <string.h>

#define MEJIRO_MAX_KEYS 32
static uint16_t chord[MEJIRO_MAX_KEYS];
static uint16_t held[MEJIRO_MAX_KEYS];
static uint8_t chord_len, held_len, down_count, previous_down;
static bool chord_active, new_press;

bool mejiro_first_up_chord_send = true;

bool is_stn_key(uint16_t kc) {
    switch (kc) {
        case STN_N1: case STN_S1: case STN_TL: case STN_PL: case STN_HL: case STN_ST1: case STN_ST3:
        case STN_FR: case STN_PR: case STN_LR: case STN_TR: case STN_DR:
        case STN_N2: case STN_S2: case STN_KL: case STN_WL: case STN_RL: case STN_ST2: case STN_ST4:
        case STN_RR: case STN_BR: case STN_GR: case STN_SR: case STN_ZR:
        case STN_N3: case STN_N4: case STN_A: case STN_O: case STN_E: case STN_U: return true;
        default: return false;
    }
}

static bool contains(const uint16_t *keys, uint8_t len, uint16_t key) {
    for (uint8_t i = 0; i < len; ++i) if (keys[i] == key) return true;
    return false;
}

static void append(uint16_t *keys, uint8_t *len, uint16_t key) {
    if (*len < MEJIRO_MAX_KEYS && !contains(keys, *len, key)) keys[(*len)++] = key;
}

static void remove_held(uint16_t key) {
    for (uint8_t i = 0; i < held_len; ++i) {
        if (held[i] == key) {
            memmove(held + i, held + i + 1, (held_len - i - 1) * sizeof(*held));
            --held_len;
            return;
        }
    }
}

static void reset_chord(void) {
    chord_len = 0;
    chord_active = false;
    new_press = false;
}

static void seed_from_held(void) {
    if (!held_len) return;
    chord_active = true;
    for (uint8_t i = 0; i < held_len; ++i) append(chord, &chord_len, held[i]);
}

// Bit order is the physical-to-logical Mejiro layout. Duplicate steno keys
// intentionally share a bit, while the Rust parser receives each bit once.
static uint8_t steno_bit(uint16_t kc) {
    switch (kc) {
        case STN_N1: case STN_N2: return 0;
        case STN_S1: case STN_S2: return 1;
        case STN_TL: return 2;
        case STN_KL: return 3;
        case STN_WL: return 4;
        case STN_PL: return 5;
        case STN_HL: return 6;
        case STN_RL: return 7;
        case STN_ST1: case STN_ST2: return 8;
        case STN_N3: return 9;
        case STN_A: return 10;
        case STN_O: return 11;
        case STN_SR: case STN_TR: return 12;
        case STN_LR: return 13;
        case STN_GR: return 14;
        case STN_BR: return 15;
        case STN_PR: return 16;
        case STN_FR: return 17;
        case STN_RR: return 18;
        case STN_ST3: case STN_ST4: return 19;
        case STN_N4: return 20;
        case STN_U: return 21;
        case STN_E: return 22;
        case STN_DR: case STN_ZR: return 23;
        default: return 0xff;
    }
}

static void commit_chord(void) {
    uint32_t bits = 0;
    for (uint8_t i = 0; i < chord_len; ++i) {
        uint8_t bit = steno_bit(chord[i]);
        if (bit != 0xff) bits |= UINT32_C(1) << bit;
    }
    if (!bits) return;
    static const char left[] = "#STKNYIAUntk";
    static const char right[] = "STKNYIAUntk*";
    char stroke[26];
    uint8_t len = 0;
    for (uint8_t i = 0; i < 12; ++i) if (bits & (UINT32_C(1) << i)) stroke[len++] = left[i];
    stroke[len++] = '-';
    for (uint8_t i = 0; i < 12; ++i) if (bits & (UINT32_C(1) << (i + 12))) stroke[len++] = right[i];
    // Macro replay is spelled #n, while #n-* starts/stops recording.
    if (!(bits & UINT32_C(0xfff1fe)) && (bits & 1) && (bits & (UINT32_C(7) << 9))) --len;
    stroke[len] = '\0';
    mejiro_engine_process(stroke, len);
}

void mejiro_on_press(uint16_t kc) {
    append(held, &held_len, kc);
    ++down_count;
    if (down_count > previous_down) {
        chord_active = true;
        append(chord, &chord_len, kc);
        new_press = true;
    }
    previous_down = down_count;
}

void mejiro_on_release(uint16_t kc) {
    remove_held(kc);
    if (down_count) --down_count;
    if (mejiro_first_up_chord_send && !new_press) {
        reset_chord();
        seed_from_held();
    }
    bool commit = mejiro_first_up_chord_send ? (new_press && down_count < previous_down)
                                              : (down_count == 0 && previous_down > 0);
    if (chord_active && commit && chord_len) {
        commit_chord();
        reset_chord();
        if (mejiro_first_up_chord_send && down_count) seed_from_held();
    }
    previous_down = down_count;
}

void mejiro_reset_state(void) {
    reset_chord();
    held_len = down_count = previous_down = 0;
    mejiro_engine_reset();
}
