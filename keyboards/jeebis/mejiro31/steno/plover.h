// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include QMK_KEYBOARD_H
#include "steno_keycodes.h"

// Existing STN_* assignments remain usable by both the onboard engine and HID.
void mejiro_plover_key(uint16_t keycode, bool pressed);
void mejiro_plover_reset(void);
