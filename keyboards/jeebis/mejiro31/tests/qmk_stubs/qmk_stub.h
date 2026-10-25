// SPDX-License-Identifier: GPL-2.0-or-later
// Host adapter tests use real QMK keycode definitions, fake hardware/report IO.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "keycodes.h"
#include "quantum_keycodes.h"
#include "modifiers.h"
#include "steno_keycodes.h"
#define MOD_BIT(code) (1U << ((code) & 7))
#define MATRIX_ROWS 4
#define MATRIX_COLS 12
#define PROGMEM
#define memcpy_P memcpy
#define MJI_COMBO_MS 40
#define MJI_CONTINUATION_MS 15
#define MJI_LONG_HOLD_MS 200
typedef uint32_t layer_state_t;
typedef struct {
    struct {
        struct {
            uint8_t col, row;
        } key;
        bool     pressed;
        uint16_t time;
    } event;
} keyrecord_t;
void     set_mods(uint8_t);
void     set_weak_mods(uint8_t);
void     clear_oneshot_mods(void);
void     set_oneshot_mods(uint8_t);
void     send_keyboard_report(void);
void     clear_keyboard(void);
void     register_code(uint8_t);
void     unregister_code(uint8_t);
void     tap_code16(uint16_t);
void host_consumer_send(uint16_t);
void host_system_send(uint16_t);
static inline uint16_t KEYCODE2CONSUMER(uint8_t code) {
    return code == KC_VOLU ? 0xe9 : code == KC_VOLD ? 0xea : code == KC_MUTE ? 0xe2 : 0;
}
static inline uint16_t KEYCODE2SYSTEM(uint8_t code) {
    return code >= KC_PWR && code <= KC_WAKE ? 0x81 + code - KC_PWR : 0;
}
uint8_t  get_mods(void);
uint8_t  get_weak_mods(void);
uint8_t  get_oneshot_mods(void);
void     del_mods(uint8_t);
void     del_weak_mods(uint8_t);
void     layer_clear(void);
void     default_layer_set(layer_state_t);
void     reset_keyboard(void);
uint32_t timer_read32(void);

#define NKRO_ENABLE
extern struct stub_keymap_config {
    bool nkro;
} keymap_config;
