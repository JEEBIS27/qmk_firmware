/*
 * Mejiro (メジロ式) physical steno chord capture for QMK
 */
#pragma once

#include QMK_KEYBOARD_H
#include "steno_keycodes.h"

// Public API
// true: 何かキーが離された時点で確定（First Up Chord Send）
// false: すべてのキーが離された時点で確定
extern bool mejiro_first_up_chord_send;

bool is_stn_key(uint16_t kc);
void mejiro_on_press(uint16_t kc);
void mejiro_on_release(uint16_t kc);
void mejiro_reset_state(void);

// Host encoding bridge supplied by the QMK adapter, using the shared resolver.
bool mejiro_steno_jis_host(void);
void mejiro_steno_tap(uint16_t code);
