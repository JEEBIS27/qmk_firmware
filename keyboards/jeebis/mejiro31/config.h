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

// Vial identity is shared by every keymap.
#include "qmk/vial_config.h"
