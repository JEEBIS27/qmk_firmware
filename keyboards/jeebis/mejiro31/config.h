// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
// Six Tap + six Hold maps; storage layers 12/13 retain six extra combos; 14/15 store four thumb combos.
#define DYNAMIC_KEYMAP_LAYER_COUNT 16
// Keep Vial's keymap/macro area below 4 KiB; two 4 KiB dictionary banks
// occupy the remainder of the 12 KiB logical EEPROM.
#define DYNAMIC_KEYMAP_EEPROM_MAX_ADDR 4095
#define WEAR_LEVELING_BACKING_SIZE 24576
#define MJI_COMBO_MS 150
#define MJI_CONTINUATION_MS 50
#define MJI_LONG_HOLD_MS 300
#define NKRO_DEFAULT_ON true

// All Tap/Hold decisions belong to the unified input engine.
#define NO_ACTION_TAPPING

// Vial identity is shared by every keymap.
#include "qmk/vial_config.h"
