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

// Include layout definitions
#include "alt_layouts_definitions.h"

/**
 * Alternative Layout Configuration
 * This file maps each keymap to its respective layout definition
 * and language setting
 *
 * Define the keymap name to get the appropriate layout and settings
 */

// Layout name enumeration
typedef enum {
    ALT_LAYOUT_NONE = 0,
    ALT_LAYOUT_QWERTY,
    ALT_LAYOUT_GRAPHITE,
    ALT_LAYOUT_COLEMAK,
    ALT_LAYOUT_DVORAK,
    ALT_LAYOUT_WORKMAN,
    ALT_LAYOUT_COLEMAK_DH,
    ALT_LAYOUT_HANDSDOWN_NEU,
    ALT_LAYOUT_STURDY,
    ALT_LAYOUT_ENGRAM,
    ALT_LAYOUT_GALLIUM,
    ALT_LAYOUT_CANARY,
    ALT_LAYOUT_ASTARTE,
    ALT_LAYOUT_BOO,
    ALT_LAYOUT_EUCALYN,
    ALT_LAYOUT_EUCALYN_BIACCO,
    ALT_LAYOUT_MERLIN,
    ALT_LAYOUT_O24,
    ALT_LAYOUT_TOMISUKE,
} alt_layout_t;

// Keymap name to layout mapping
// With jp_default and en_default keymaps, we use graphite layout
#if defined(KEYMAP_JP_DEFAULT) || defined(KEYMAP_EN_DEFAULT)
    #define ALT_LAYOUT_ID ALT_LAYOUT_GRAPHITE
    #define ALT_LAYOUT_MAPPINGS graphite
#else
    // Default fallback
    #define ALT_LAYOUT_ID ALT_LAYOUT_GRAPHITE
    #define ALT_LAYOUT_MAPPINGS graphite
#endif

// Helper function to get layout name as string (optional)
static inline const char* alt_layout_name(alt_layout_t layout) {
    switch (layout) {
        case ALT_LAYOUT_NONE:            return "none";
        case ALT_LAYOUT_QWERTY:          return "qwerty";
        case ALT_LAYOUT_GRAPHITE:        return "graphite";
        case ALT_LAYOUT_COLEMAK:         return "colemak";
        case ALT_LAYOUT_DVORAK:          return "dvorak";
        case ALT_LAYOUT_WORKMAN:         return "workman";
        case ALT_LAYOUT_COLEMAK_DH:      return "colemak-dh";
        case ALT_LAYOUT_HANDSDOWN_NEU:   return "handsdown-neu";
        case ALT_LAYOUT_STURDY:          return "sturdy";
        case ALT_LAYOUT_ENGRAM:          return "engram";
        case ALT_LAYOUT_GALLIUM:         return "gallium";
        case ALT_LAYOUT_CANARY:          return "canary";
        case ALT_LAYOUT_ASTARTE:         return "astarte";
        case ALT_LAYOUT_BOO:             return "boo";
        case ALT_LAYOUT_EUCALYN:         return "eucalyn";
        case ALT_LAYOUT_EUCALYN_BIACCO:  return "eucalyn-biacco";
        case ALT_LAYOUT_MERLIN:          return "merlin";
        case ALT_LAYOUT_O24:             return "o24";
        case ALT_LAYOUT_TOMISUKE:        return "tomisuke";
        default:                         return "unknown";
    }
}
