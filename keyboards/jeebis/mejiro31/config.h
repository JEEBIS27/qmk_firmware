// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
// Six logical Tap maps and their six matching Hold maps.
#define DYNAMIC_KEYMAP_LAYER_COUNT 12
#define MJI_COMBO_MS 150
#define MJI_CONTINUATION_MS 50
#define MJI_LONG_HOLD_MS 300
#define NKRO_DEFAULT_ON true

// All Tap/Hold decisions belong to the unified input engine.
#define NO_ACTION_TAPPING

// Every Mejiro31 keymap uses the same Vial identity and physical unlock chord.
#define VIAL_KEYBOARD_UID {0x4d, 0x33, 0x31, 0x57, 0x45, 0x42, 0x30, 0x31}
#define VIAL_UNLOCK_COMBO_ROWS {0, 0}
#define VIAL_UNLOCK_COMBO_COLS {0, 11}
#define VIA_CUSTOM_LIGHTING_ENABLE
