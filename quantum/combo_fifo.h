/*
Copyright 2025 JEEBIS <jeebis.iox@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include QMK_KEYBOARD_H
#include "jis_transform.h"

#define COMBO_FIFO_LEN       30  // FIFO length
#define COMBO_TIMEOUT_MS     200 // Combo timeout in ms (COMBO_TERM)

void tap_code16_with_shift(uint16_t kc);
void register_code16_with_shift(uint16_t kc);
void unregister_code16_with_shift(uint16_t kc);
void register_code16_without_shift(uint16_t kc);
void unregister_code16_without_shift(uint16_t kc);

typedef enum {
    HOLD_REG_NONE = 0,
    HOLD_REG_NORMAL,
    HOLD_REG_WITH_SHIFT,
    HOLD_REG_WITHOUT_SHIFT,
} hold_register_mode_t;

typedef struct {
    keypos_t key;
    uint16_t keycode;
    uint16_t time_pressed;
    uint8_t  layer;
    bool     released;
} combo_event_t;

typedef struct {
    uint16_t a;
    uint16_t b;
    uint16_t out;
    uint8_t  layer;
} combo_pair_t;

typedef struct {
    uint16_t keycode;
    uint16_t time_confirmed;
    bool     is_held;
    hold_register_mode_t register_mode;
    uint16_t source_key_a;
    uint16_t source_key_b;
    bool     source_a_pressed;
    bool     source_b_pressed;
} hold_state_t;

// Globals defined in process_combo.c
extern combo_event_t combo_fifo[COMBO_FIFO_LEN];
extern uint8_t combo_fifo_len;
extern hold_state_t hold_state;

// Combo pair definitions (keymap-specific)
extern const combo_pair_t combo_pairs[];
extern uint8_t combo_pair_count;

// Combo candidate check (optional override)
extern bool is_combo_candidate(uint16_t keycode);

// FIFO enqueue hook (optional override)
void combo_fifo_on_enqueue(const combo_event_t *event, uint8_t fifo_len);

/**
 * Default combo candidate check
 * @param keycode Keycode
 * @param exclude_kc Keycode to exclude (0 = none)
 * @return true if combo candidate
 */
static inline bool is_combo_candidate_default(uint16_t keycode, uint16_t exclude_kc) {
    if (exclude_kc != 0 && keycode == exclude_kc) return false;
    // Exclude modifiers
    switch (keycode) {
        case KC_LCTL: case KC_RCTL:
        case KC_LGUI: case KC_RGUI:
        case KC_LALT: case KC_RALT:
        case KC_LSFT: case KC_RSFT:
            return false;
    }

    uint16_t base = keycode;
    for (uint8_t i = 0; i < combo_pair_count; i++) {
        combo_pair_t pair;
        memcpy_P(&pair, &combo_pairs[i], sizeof(pair));
        if (pair.a == base || pair.b == base) {
            return true;
        }
    }
    return false;
}

// Key transform functions (keymap-specific)
typedef uint16_t (*key_transform_fn_t)(uint16_t kc);

// Extended key transform
typedef struct {
    uint16_t keycode;
    bool     needs_unshift;
} transformed_key_t;

typedef transformed_key_t (*key_transform_extended_fn_t)(uint16_t kc, bool shifted, uint8_t layer);

bool combo_fifo_custom_action(uint16_t keycode, bool shifted, bool needs_unshift, bool is_hold);

/**
 * Find combo pair
 * @param a Keycode A
 * @param b Keycode B
 * @return Pointer to combo pair or NULL
 */
static inline const combo_pair_t *find_combo(uint16_t a, uint16_t b) {
    for (uint8_t i = 0; i < combo_pair_count; i++) {
        combo_pair_t pair;
        memcpy_P(&pair, &combo_pairs[i], sizeof(pair));
        if ((pair.a == a && pair.b == b) || (pair.a == b && pair.b == a)) {
            return &combo_pairs[i];
        }
    }
    return NULL;
}

/**
 * Remove FIFO element at index
 * @param idx Index
 */
static inline void fifo_remove(uint8_t idx) {
    if (idx >= combo_fifo_len) return;
    for (uint8_t i = idx; i + 1 < combo_fifo_len; i++) {
        combo_fifo[i] = combo_fifo[i + 1];
    }
    combo_fifo_len--;
}

/**
 * Clear hold state
 */
static inline void clear_hold_state(void) {
    if (hold_state.is_held) {
        switch (hold_state.register_mode) {
            case HOLD_REG_WITH_SHIFT:
                unregister_code16_with_shift(hold_state.keycode);
                break;
            case HOLD_REG_WITHOUT_SHIFT:
                unregister_code16_without_shift(hold_state.keycode);
                break;
            case HOLD_REG_NORMAL:
                unregister_code16(hold_state.keycode);
                break;
            case HOLD_REG_NONE:
            default:
                break;
        }
    }
    hold_state.is_held = false;
    hold_state.keycode = 0;
    hold_state.register_mode = HOLD_REG_NONE;
}

/**
 * Basic combo resolution (simple transform)
 * @param transform_fn Key transform
 * @return true if resolved
 */
static inline bool resolve_combo_head_basic(key_transform_fn_t transform_fn) {
    if (combo_fifo_len < 2) return false;

    uint16_t head_kc    = combo_fifo[0].keycode;

    for (uint8_t i = 1; i < combo_fifo_len; i++) {
        uint16_t other_kc    = combo_fifo[i].keycode;

        const combo_pair_t *hit = find_combo(head_kc, other_kc);
        if (hit) {
            combo_pair_t pair;
            memcpy_P(&pair, hit, sizeof(pair));

            uint16_t out_kc = transform_fn(pair.out);
            bool head_pressed  = !combo_fifo[0].released;
            bool other_pressed = !combo_fifo[i].released;

            if (!head_pressed || !other_pressed) {
                clear_hold_state();
                tap_code16(out_kc);
                fifo_remove(i);
                fifo_remove(0);
                return true;
            }

            clear_hold_state();
            hold_state.keycode = out_kc;
            hold_state.time_confirmed = timer_read();
            hold_state.is_held = true;
            hold_state.register_mode = HOLD_REG_NORMAL;
            hold_state.source_key_a = head_kc;
            hold_state.source_key_b = other_kc;
            hold_state.source_a_pressed = head_pressed;
            hold_state.source_b_pressed = other_pressed;

            register_code16(out_kc);
            fifo_remove(i);
            fifo_remove(0);
            return true;
        }
    }
    return false;
}

/**
 * @param transform_fn Extended key transform
 * @return true if resolved
 */
static inline bool resolve_combo_head_extended(key_transform_extended_fn_t transform_fn) {
    if (combo_fifo_len < 2) return false;

    uint16_t head_kc    = combo_fifo[0].keycode;
    uint8_t  head_layer = combo_fifo[0].layer;
    bool shifted = (get_mods() & MOD_MASK_SHIFT);

    for (uint8_t i = 1; i < combo_fifo_len; i++) {
        uint16_t other_kc    = combo_fifo[i].keycode;

        const combo_pair_t *hit = find_combo(head_kc, other_kc);
        if (hit) {
            combo_pair_t pair;
            memcpy_P(&pair, hit, sizeof(pair));

            transformed_key_t transformed = transform_fn(pair.out, shifted, head_layer);

            bool head_pressed  = !combo_fifo[0].released;
            bool other_pressed = !combo_fifo[i].released;

            if (!head_pressed || !other_pressed) {
                clear_hold_state();
                if (combo_fifo_custom_action(transformed.keycode, shifted, transformed.needs_unshift, false)) {
                    fifo_remove(i);
                    fifo_remove(0);
                    return true;
                }
                if (transformed.needs_unshift) {
                    tap_code16_unshifted(transformed.keycode);
                } else if (shifted) {
                    tap_code16_with_shift(transformed.keycode);
                } else {
                    tap_code16_unshifted(transformed.keycode);
                }
                fifo_remove(i);
                fifo_remove(0);
                return true;
            }

            clear_hold_state();
            if (combo_fifo_custom_action(transformed.keycode, shifted, transformed.needs_unshift, true)) {
                fifo_remove(i);
                fifo_remove(0);
                return true;
            }
            hold_state.keycode = transformed.keycode;
            hold_state.time_confirmed = timer_read();
            hold_state.is_held = true;
            if (transformed.needs_unshift) {
                hold_state.register_mode = HOLD_REG_WITHOUT_SHIFT;
            } else if (shifted) {
                hold_state.register_mode = HOLD_REG_WITH_SHIFT;
            } else {
                hold_state.register_mode = HOLD_REG_NORMAL;
            }
            hold_state.source_key_a = head_kc;
            hold_state.source_key_b = other_kc;
            hold_state.source_a_pressed = head_pressed;
            hold_state.source_b_pressed = other_pressed;

            if (transformed.needs_unshift) {
                register_code16_without_shift(transformed.keycode);
            } else if (shifted) {
                register_code16_with_shift(transformed.keycode);
            } else {
                register_code16(transformed.keycode);
            }
            fifo_remove(i);
            fifo_remove(0);
            return true;
        }
    }
    return false;
}

/**
 * @param transform_fn Key transform
 */
static inline void combo_fifo_service_basic(key_transform_fn_t transform_fn) {
    while (combo_fifo_len > 0) {
        if (combo_fifo_len == 1) {
            if (combo_fifo[0].released) {
                uint16_t base_kc = combo_fifo[0].keycode;
                uint16_t out_kc = transform_fn(base_kc);
                tap_code16(out_kc);
                fifo_remove(0);
                continue;
            }
            if (timer_elapsed(combo_fifo[0].time_pressed) > COMBO_TIMEOUT_MS) {
                uint16_t base_kc = combo_fifo[0].keycode;
                uint16_t out_kc = transform_fn(base_kc);
                clear_hold_state();
                hold_state.keycode = out_kc;
                hold_state.time_confirmed = timer_read();
                hold_state.is_held = true;
                hold_state.register_mode = HOLD_REG_NORMAL;
                hold_state.source_key_a = base_kc;
                hold_state.source_key_b = 0;
                hold_state.source_a_pressed = true;
                hold_state.source_b_pressed = false;
                register_code16(out_kc);
                fifo_remove(0);
                continue;
            }
            break;
        }
        if (combo_fifo_len >= 2) {
            if (resolve_combo_head_basic(transform_fn)) {
                continue;
            }
            if (combo_fifo[0].released) {
                clear_hold_state();
                uint16_t base_kc = combo_fifo[0].keycode;
                uint16_t out_kc = transform_fn(base_kc);
                fifo_remove(0);
                tap_code16(out_kc);
                continue;
            } else {
                if (timer_elapsed(combo_fifo[0].time_pressed) > COMBO_TIMEOUT_MS) {
                    uint16_t base_kc = combo_fifo[0].keycode;
                    uint16_t out_kc = transform_fn(base_kc);
                    tap_code16(out_kc);
                    fifo_remove(0);
                    continue;
                }
                break;
            }
        }
        break;
    }
}

/**
 * @param transform_fn Extended key transform
 */
static inline void combo_fifo_service_extended(key_transform_extended_fn_t transform_fn) {
    while (combo_fifo_len > 0) {
        if (combo_fifo_len == 1) {
            if (combo_fifo[0].released) {
                uint16_t base_kc = combo_fifo[0].keycode;

                uint8_t layer = combo_fifo[0].layer;
                bool shifted = (get_mods() & MOD_MASK_SHIFT);
                transformed_key_t transformed = transform_fn(base_kc, shifted, layer);
                if (combo_fifo_custom_action(transformed.keycode, shifted, transformed.needs_unshift, false)) {
                    fifo_remove(0);
                    continue;
                }
                if (transformed.needs_unshift) {
                    tap_code16_unshifted(transformed.keycode);
                } else if (shifted) {
                    tap_code16_with_shift(transformed.keycode);
                } else {
                    tap_code16_unshifted(transformed.keycode);
                }
                fifo_remove(0);
                continue;
            }
            if (timer_elapsed(combo_fifo[0].time_pressed) > COMBO_TIMEOUT_MS) {
                uint16_t base_kc = combo_fifo[0].keycode;

                uint8_t layer = combo_fifo[0].layer;
                bool shifted = (get_mods() & MOD_MASK_SHIFT);
                transformed_key_t transformed = transform_fn(base_kc, shifted, layer);
                if (combo_fifo_custom_action(transformed.keycode, shifted, transformed.needs_unshift, true)) {
                    clear_hold_state();
                    fifo_remove(0);
                    continue;
                }
                clear_hold_state();
                hold_state.keycode = transformed.keycode;
                hold_state.time_confirmed = timer_read();
                hold_state.is_held = true;
                if (transformed.needs_unshift) {
                    hold_state.register_mode = HOLD_REG_WITHOUT_SHIFT;
                    register_code16_without_shift(transformed.keycode);
                } else if (shifted) {
                    hold_state.register_mode = HOLD_REG_WITH_SHIFT;
                    register_code16_with_shift(transformed.keycode);
                } else {
                    hold_state.register_mode = HOLD_REG_NORMAL;
                    register_code16(transformed.keycode);
                }
                hold_state.source_key_a = base_kc;
                hold_state.source_key_b = 0;
                hold_state.source_a_pressed = true;
                hold_state.source_b_pressed = false;
                fifo_remove(0);
                continue;
            }
            break;
        }
        if (combo_fifo_len >= 2) {
            if (resolve_combo_head_extended(transform_fn)) {
                continue;
            }
            if (combo_fifo[0].released) {
                clear_hold_state();
                uint16_t base_kc = combo_fifo[0].keycode;
                uint8_t layer = combo_fifo[0].layer;
                bool shifted = (get_mods() & MOD_MASK_SHIFT);
                transformed_key_t transformed = transform_fn(base_kc, shifted, layer);
                if (!combo_fifo_custom_action(transformed.keycode, shifted, transformed.needs_unshift, false)) {
                    if (transformed.needs_unshift) {
                        tap_code16_unshifted(transformed.keycode);
                    } else if (shifted) {
                        tap_code16_with_shift(transformed.keycode);
                    } else {
                        tap_code16_unshifted(transformed.keycode);
                    }
                }
                fifo_remove(0);
                continue;
            } else {
                if (timer_elapsed(combo_fifo[0].time_pressed) > COMBO_TIMEOUT_MS) {
                    uint16_t base_kc = combo_fifo[0].keycode;
                    uint8_t layer = combo_fifo[0].layer;
                    bool shifted = (get_mods() & MOD_MASK_SHIFT);
                    transformed_key_t transformed = transform_fn(base_kc, shifted, layer);
                    if (transformed.needs_unshift) {
                        tap_code16_unshifted(transformed.keycode);
                    } else if (shifted) {
                        tap_code16_with_shift(transformed.keycode);
                    } else {
                        tap_code16_unshifted(transformed.keycode);
                    }
                    fifo_remove(0);
                    continue;
                }
                break;
            }
        }
        break;
    }
}
