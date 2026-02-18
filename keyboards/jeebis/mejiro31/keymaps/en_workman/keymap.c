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

/*---------------------------------------------------------------------------------------------------*/
/*----------------------------------------------Initial Setup----------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/

#include QMK_KEYBOARD_H
#include "os_detection.h"
#include "combo_fifo.h"
#include "alt_layout.h"

enum layer_names {
    _QWERTY = 0,
    _GEMINI,
    _NUMBER,
    _FUNCTION,
};

enum custom_keycodes {
    KC_DZ = SAFE_RANGE,  // 00 key
    KC_TZ,               // 000 key
    TG_ALT,              // Alternative Layout toggle key
    TG_SBL,              // Mejiro31 Symbol Layout toggle key
};

#define MT_SPC KC_LSFT
#define MT_ENT KC_RSFT
#define MT_ESC MT(MOD_LGUI, KC_ESC)
#define MO_FUN MO(_FUNCTION)
#define MT_TGL LT(_NUMBER, KC_F24)

bool is_sbl_mode = true;
static bool is_mac = false;
static bool os_detected = false;

static uint16_t dz_timer = 0;
static bool dz_delayed = false;
static uint8_t dz_fifo_len_at_press = 0;  // DZ key press time combo FIFO length
static uint16_t lshift_timer = 0;      // L_shift press time
static uint16_t rshift_timer = 0;      // R_shift press time
static bool lshift_has_key = false;    // Whether a new key was pressed during L_shift hold
static bool rshift_has_key = false;    // Whether a new key was pressed during R_shift hold

typedef struct {
    bool pressed;
    uint16_t timer;
} toggle_hold_state_t;
static toggle_hold_state_t tg_alt_state = {false, 0};
static toggle_hold_state_t tg_sbl_state = {false, 0};

typedef union {
    uint32_t raw;
    struct {
        bool alt_mode : 1;
        bool sbl_mode : 1;
    };
} user_config_t;

static user_config_t user_config;

void eeconfig_init_user(void) {
    user_config.raw = 0;
    user_config.alt_mode = true;
    user_config.sbl_mode = true;
    eeconfig_update_user(user_config.raw);
    steno_set_mode(STENO_MODE_GEMINI);
}

void keyboard_post_init_user(void) {
    os_variant_t os = detected_host_os();
    is_mac = (os == OS_MACOS || os == OS_IOS);
    user_config.raw = eeconfig_read_user();
    is_alt_mode = (user_config.alt_mode);
    is_sbl_mode = (user_config.sbl_mode);
    default_layer_set((layer_state_t)1UL << _QWERTY);
    layer_clear();
    layer_move(_QWERTY);
    default_layer = 0;
}

void tap_code16_unshifted(uint16_t keycode) {
    uint8_t mods = get_mods();
    uint8_t shift_mods = mods & MOD_MASK_SHIFT;
    if (shift_mods) {
        unregister_mods(shift_mods);
    }
    tap_code16(keycode);
    if (shift_mods) {
        register_mods(shift_mods);
    }
}

/**
 * @param kc
 */
void tap_code16_with_shift(uint16_t kc) {
    uint8_t saved_mods      = get_mods();
    uint8_t saved_weak_mods = get_weak_mods();
    uint8_t saved_osm       = get_oneshot_mods();

    add_mods(MOD_LSFT);
    send_keyboard_report();

    tap_code16(kc);

    set_mods(saved_mods);
    set_weak_mods(saved_weak_mods);
    set_oneshot_mods(saved_osm);
    send_keyboard_report();
}

/**
 * @param kc
 */
void register_code16_with_shift(uint16_t kc) {
    add_mods(MOD_LSFT);
    send_keyboard_report();
    register_code16(kc);
}

/**
 * @param kc
 */
void unregister_code16_with_shift(uint16_t kc) {
    unregister_code16(kc);
    del_mods(MOD_LSFT);
    send_keyboard_report();
}

/*---------------------------------------------------------------------------------------------------*/
/*-------------------------------------Mejiro31 Symbol Layout----------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/

typedef struct {
    uint16_t base;
    uint16_t unshifted;
    uint16_t shifted;
    uint8_t layer;
} sbl_mapping_t;

static const sbl_mapping_t sbl_mappings[] PROGMEM = {

    // QWERTY
    // ┌─────┬─────┬─────┬─────┬─────┬─────┐             ┌─────┬─────┬─────┬─────┬─────┬─────┐
    // │  `  │  q  │  w  │  e  │  r ESC t  │             │  y DEL u  │  i  │  o  │  p  │  -  │
    // ├─────┼──a──┼──s──┼──d──┼──f──┼──g──┤             ├──h──┼──j──┼──k──┼──l──┼──;──┼──'──┤
    // │ ESC │  z  │  x  │  c  │  v TAB b  │             │  n BSP m  │  ,  │  .  │  /  │  \  │
    // └─────┴─────┴─────┴─────┴─────┴─────┘             └─────┴─────┴─────┴─────┴─────┴─────┘
    //                         ┌───────────┐             ┌───────────┐
    //                         │   SandS   │             │   EandS   │
    //                         ├─────┬─────┤   ┌─────┐   ├─────┬─────┤
    //                         │ ALT │ CTL │   │MT_TG│   │  !  #  ?  │
    //                         └─────┴─────┘   └─────┘   └─────┴─────┘
    // QWERTY Shifted
    // ┌─────┬─────┬─────┬─────┬─────┬─────┐             ┌─────┬─────┬─────┬─────┬─────┬─────┐
    // │  ~  │  Q  │  W  │  E  │  R ESC T  │             │  Y DEL U  │  I  │  O  │  P  │  _  │
    // ├─────┼──A──┼──S──┼──D──┼──F──┼──G──┤             ├──H──┼──J──┼──K──┼──L──┼──:──┼──"──┤
    // │ ESC │  Z  │  X  │  C  │  V TAB B  │             │  N BSP M  │  <  │  >  │  ?  │  |  │
    // └─────┴─────┴─────┴─────┴─────┴─────┘             └─────┴─────┴─────┴─────┴─────┴─────┘
    //                         ┌───────────┐             ┌───────────┐
    //                         │   SandS   │             │   EandS   │
    //                         ├─────┬─────┤   ┌─────┐   ├─────┬─────┤
    //                         │ ALT │ CTL │   │MT_TG│   │  &  @  |  │
    //                         └─────┴─────┘   └─────┘   └─────┴─────┘
    // NUMBER
    // ┌─────┬─────┬─────┬─────┬─────┬─────┐             ┌─────┬─────┬─────┬─────┬─────┬─────┐
    // │  `  │ 00  │  1  │  2  │  3  │  -  │             │ PGU │ HOM │  ↑  │ END │ CAP │ ALT │
    // ├─────┼─────┼──4──┼──5──┼──6──┼──,──┤             ├─────┼─────┼─────┼─────┼─────┼─────┤
    // │ ESC │  0  │  7  │  8  │  9  │  .  │             │ PGD │  ←  │  ↓  │  →  │ GUI │MO_FN│
    // └─────┴─────┴─────┴─────┴─────┴─────┘             └─────┴─────┴─────┴─────┴─────┴─────┘
    //                         ┌───────────┐             ┌───────────┐
    //                         │   SandS   │             │   EandS   │
    //                         ├─────┬─────┤   ┌─────┐   ├─────┬─────┤
    //                         │ ALT │ CTL │   │MT_TG│   │  !  #  ?  │
    //                         └─────┴─────┘   └─────┘   └─────┴─────┘
    // NUMBER Shifted
    // ┌─────┬─────┬─────┬─────┬─────┬─────┐             ┌─────┬─────┬─────┬─────┬─────┬─────┐
    // │  ~  │  %  │  {  │  [  │  (  │  <  │             │ PGU │ HOM │  ↑  │ END │ CAP │ ALT │
    // ├─────┼──/──┼──*──┼──=──┼──+──┼──^──┤             ├─────┼─────┼─────┼─────┼─────┼─────┤
    // │ ESC │  $  │  }  │  ]  │  )  │  >  │             │ PGD │  ←  │  ↓  │  →  │ GUI │MO_FN│
    // └─────┴─────┴─────┴─────┴─────┴─────┘             └─────┴─────┴─────┴─────┴─────┴─────┘
    //                         ┌───────────┐             ┌───────────┐
    //                         │   SandS   │             │   EandS   │
    //                         ├─────┬─────┤   ┌─────┐   ├─────┬─────┤
    //                         │ ALT │ CTL │   │MT_TG│   │  &  @  |  │
    //                         └─────┴─────┘   └─────┘   └─────┴─────┘

    {KC_LBRC, KC_EXLM, KC_AMPR,    _QWERTY},  // [ / ! / &
    {KC_RBRC, KC_QUES, KC_PIPE,    _QWERTY},  // ] / ? / |
    {KC_EQL,  KC_HASH, KC_AT,      _QWERTY},  // = / # / @

    {KC_LBRC, KC_EXLM, KC_AMPR,    _NUMBER},  // [ / ! / &
    {KC_RBRC, KC_QUES, KC_PIPE,    _NUMBER},  // ] / ? / |
    {KC_EQL,  KC_HASH, KC_AT,      _NUMBER},  // = / # / @

    {KC_DZ,   KC_DZ,   KC_PERC,    _NUMBER},  // 00 / 00 / %
    {KC_1,    KC_1,    KC_LCBR,    _NUMBER},  // 1 / 1 / {
    {KC_2,    KC_2,    KC_LBRC,    _NUMBER},  // 2 / 2 / [
    {KC_3,    KC_3,    KC_LPRN,    _NUMBER},  // 3 / 3 / (
    {KC_MINS, KC_MINS, KC_LABK,    _NUMBER},  // - / - / <
    {KC_TZ,   KC_TZ,   KC_SLSH,    _NUMBER},  // 000 / 000 / /
    {KC_4,    KC_4,    KC_ASTR,    _NUMBER},  // 4 / 4 / *
    {KC_5,    KC_5,    KC_EQL,     _NUMBER},  // 5 / 5 / =
    {KC_6,    KC_6,    KC_PLUS,    _NUMBER},  // 6 / 6 / +
    {KC_COMM, KC_COMM, KC_CIRC,    _NUMBER},  // , / , / ^
    {KC_0,    KC_0,    KC_DLR,     _NUMBER},  // 0 / 0 / $
    {KC_7,    KC_7,    KC_RCBR,    _NUMBER},  // 7 / 7 / }
    {KC_8,    KC_8,    KC_RBRC,    _NUMBER},  // 8 / 8 / ]
    {KC_9,    KC_9,    KC_RPRN,    _NUMBER},  // 9 / 9 / )
    {KC_DOT,  KC_DOT,  KC_RABK,    _NUMBER},  // . / . / >
};

uint16_t sbl_transform(uint16_t kc, bool shifted, uint8_t layer) {
    if (!is_sbl_mode || force_qwerty_active) return kc;

    for (uint8_t i = 0; i < sizeof(sbl_mappings) / sizeof(sbl_mappings[0]); i++) {
        sbl_mapping_t mapping;
        memcpy_P(&mapping, &sbl_mappings[i], sizeof(mapping));
        if (mapping.layer != layer) continue;
        if (mapping.base == kc) {
            return shifted ? mapping.shifted : mapping.unshifted;
        }
    }

    return kc;
}

/*---------------------------------------------------------------------------------------------------*/
/*----------------------------------------Alternative Layout-----------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/

// Layout: Workman
// ┌─────┬─────┬─────┬─────┬─────┐┌─────┬─────┬─────┬─────┬─────┬─────┐
// │  q  │  d  │  r  │  w  │  b  ││  j  │  f  │  u  │  p  │  ;  │  -  │
// ├──a──┼──s──┼──h──┼──t──┼──g──┤├──y──┼──n──┼──e──┼──o──┼──i──┼──'──┤
// │  z  │  x  │  m  │  c  │  v  ││  k  │  l  │  ,  │  .  │  /  │  \  │
// └─────┴─────┴─────┴─────┴─────┘└─────┴─────┴─────┴─────┴─────┴─────┘
static const alt_mapping_t alt_mappings[] PROGMEM = {
    {KC_Q,    KC_Q,    KC_Q},
    {KC_W,    KC_D,    KC_D},
    {KC_E,    KC_R,    KC_R},
    {KC_R,    KC_W,    KC_W},
    {KC_T,    KC_B,    KC_B},
    {KC_Y,    KC_J,    KC_J},
    {KC_U,    KC_F,    KC_F},
    {KC_I,    KC_U,    KC_U},
    {KC_O,    KC_P,    KC_P},
    {KC_P,    KC_SCLN, KC_COLN},
    {KC_MINS, KC_MINS, KC_UNDS},

    {KC_A,    KC_A,    KC_A},
    {KC_S,    KC_S,    KC_S},
    {KC_D,    KC_H,    KC_H},
    {KC_F,    KC_T,    KC_T},
    {KC_G,    KC_G,    KC_G},
    {KC_H,    KC_Y,    KC_Y},
    {KC_J,    KC_N,    KC_N},
    {KC_K,    KC_E,    KC_E},
    {KC_L,    KC_O,    KC_O},
    {KC_SCLN, KC_I,    KC_I},
    {KC_QUOT, KC_QUOT, KC_DQUO},

    {KC_Z,    KC_Z,    KC_Z},
    {KC_X,    KC_X,    KC_X},
    {KC_C,    KC_M,    KC_M},
    {KC_V,    KC_C,    KC_C},
    {KC_B,    KC_V,    KC_V},
    {KC_N,    KC_K,    KC_K},
    {KC_M,    KC_L,    KC_L},
    {KC_COMM, KC_COMM, KC_LABK},
    {KC_DOT,  KC_DOT,  KC_RABK},
    {KC_SLSH, KC_SLSH, KC_QUES},
    {KC_BSLS, KC_BSLS, KC_PIPE},
};

uint16_t alt_transform(uint16_t kc, bool shifted, uint8_t layer) {
    if (!is_alt_mode || force_qwerty_active) return kc;

    if (layer != _QWERTY) return kc;

    for (uint8_t i = 0; i < sizeof(alt_mappings) / sizeof(alt_mappings[0]); i++) {
        alt_mapping_t mapping;
        memcpy_P(&mapping, &alt_mappings[i], sizeof(mapping));
        if (mapping.base == kc) {
            return shifted ? mapping.shifted : mapping.unshifted;
        }
    }

    return kc;
}

static inline transformed_key_t transform_key_extended(uint16_t kc, bool shifted, uint8_t layer) {
    uint16_t sbl_kc = sbl_transform(kc, shifted, layer);
    uint16_t alt_kc = alt_transform(sbl_kc, shifted, layer);

    bool needs_unshift = false;
    if (shifted && is_sbl_mode && !force_qwerty_active) {
        if (layer == _NUMBER || layer == _QWERTY) {
            if (sbl_kc != kc) {
                needs_unshift = true;
            }
        }
    }

    transformed_key_t result = {
        .keycode = alt_kc,
        .needs_unshift = needs_unshift,
    };
    return result;
}



static void refresh_force_qwerty_state(void) {
    uint8_t current_layer = get_highest_layer(layer_state | default_layer_state);
    bool is_number_or_function = (current_layer == _NUMBER || current_layer == _FUNCTION);

    bool should_force = mods_except_shift_active() && !is_number_or_function;
    layer_state_t qwerty_default = (layer_state_t)1UL << _QWERTY;

    if (should_force) {
        if (!force_qwerty_active || default_layer_state != qwerty_default) {
            default_layer_set(qwerty_default);
            layer_move(_QWERTY);
        }
        force_qwerty_active = true;
    } else if (force_qwerty_active) {
        layer_state_t target_default = (layer_state_t)1UL << (default_layer == 0 ? _QWERTY : _GEMINI);
        default_layer_set(target_default);
        layer_move(default_layer == 0 ? _QWERTY : _GEMINI);
        force_qwerty_active = false;
    }
}

static void toggle_alt_mode(void) {
    is_alt_mode = !is_alt_mode;
    user_config.alt_mode = is_alt_mode;
    eeconfig_update_user(user_config.raw);
}

static void toggle_sbl_mode(void) {
    is_sbl_mode = !is_sbl_mode;
    user_config.sbl_mode = is_sbl_mode;
    eeconfig_update_user(user_config.raw);
}

static bool handle_toggle_on_hold(keyrecord_t *record, toggle_hold_state_t *state, void (*toggle_fn)(void)) {
    if (record->event.pressed) {
        state->pressed = true;
        state->timer = timer_read();
        return false;
    }

    if (!state->pressed) return false;

    state->pressed = false;
    if (timer_elapsed(state->timer) >= COMBO_TIMEOUT_MS) {
        toggle_fn();
    }
    return false;
}

/*---------------------------------------------------------------------------------------------------*/
/*---------------------------------------------FIFO Combo--------------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/

// Combo definitions (unordered)
const combo_pair_t combo_pairs[] PROGMEM = {

    {KC_Q,    KC_Z,    KC_A,    _QWERTY},
    {KC_W,    KC_X,    KC_S,    _QWERTY},
    {KC_E,    KC_C,    KC_D,    _QWERTY},
    {KC_R,    KC_V,    KC_F,    _QWERTY},
    {KC_T,    KC_B,    KC_G,    _QWERTY},
    {KC_Y,    KC_N,    KC_H,    _QWERTY},
    {KC_U,    KC_M,    KC_J,    _QWERTY},
    {KC_I,    KC_COMM, KC_K,    _QWERTY},
    {KC_O,    KC_DOT,  KC_L,    _QWERTY},
    {KC_P,    KC_SLSH, KC_SCLN, _QWERTY},
    {KC_MINS, KC_BSLS, KC_QUOT, _QWERTY},
    {KC_LBRC, KC_RBRC, KC_EQL,  _QWERTY},
    {KC_V,    KC_B,    KC_TAB,  _QWERTY},
    {KC_R,    KC_T,    KC_ESC,  _QWERTY},
    {KC_N,    KC_M,    KC_BSPC, _QWERTY},
    {KC_Y,    KC_U,    KC_DEL,  _QWERTY},

    {KC_1,    KC_7,    KC_4,     _NUMBER},
    {KC_2,    KC_8,    KC_5,     _NUMBER},
    {KC_3,    KC_9,    KC_6,     _NUMBER},
    {KC_0,    KC_DZ,   KC_TZ,    _NUMBER},
    {KC_DOT,  KC_MINS, KC_COMMA, _NUMBER},
    {KC_9,    KC_DOT,  KC_TAB,   _NUMBER},
    {KC_3,    KC_MINS, KC_ESC,   _NUMBER},
    {KC_PGDN, KC_LEFT, KC_BSPC,  _NUMBER},
    {KC_PGUP, KC_HOME, KC_DEL,   _NUMBER},
};
uint8_t combo_pair_count = sizeof(combo_pairs) / sizeof(combo_pairs[0]);

bool is_combo_candidate(uint16_t keycode) {
    uint8_t mods = get_mods();
    bool shifted = (mods & MOD_MASK_SHIFT);
    if (keycode == KC_DZ) return is_sbl_mode && shifted;
    if (keycode == KC_TZ) return is_sbl_mode && shifted;
    return is_combo_candidate_default(keycode, 0);
}

/*---------------------------------------------------------------------------------------------------*/
/*----------------------------------------------Keymaps----------------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {


    // QWERTY
    // ┌─────┬─────┬─────┬─────┬─────┬─────┐             ┌─────┬─────┬─────┬─────┬─────┬─────┐
    // │  `  │  q  │  w  │  e  │  r ESC t  │             │  y DEL u  │  i  │  o  │  p  │  -  │
    // ├─────┼──a──┼──s──┼──d──┼──f──┼──g──┤             ├──h──┼──j──┼──k──┼──l──┼──;──┼──'──┤
    // │ ESC │  z  │  x  │  c  │  v TAB b  │             │  n BSP m  │  ,  │  .  │  /  │  \  │
    // └─────┴─────┴─────┴─────┴─────┴─────┘             └─────┴─────┴─────┴─────┴─────┴─────┘
    //                         ┌───────────┐             ┌───────────┐
    //                         │   SandS   │             │   EandS   │
    //                         ├─────┬─────┤   ┌─────┐   ├─────┬─────┤
    //                         │ ALT │ CTL │   │MT_TG│   │  [  =  ]  │
    //                         └─────┴─────┘   └─────┘   └─────┴─────┘
    // QWERTY
    [_QWERTY] = LAYOUT(
        KC_GRV, KC_Q, KC_W, KC_E, KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,   KC_P,    KC_MINS,
        KC_ESC, KC_Z, KC_X, KC_C, KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT, KC_SLSH, KC_BSLS,
                                  MT_SPC , MT_TGL,  MT_ENT,
                                  KC_LALT, KC_LCTL, KC_LBRC, KC_RBRC
    ),

    // GEMINI
    // ┌─────┬─────┬─────┬─────┬─────┬─────┐             ┌─────┬─────┬─────┬─────┬─────┬─────┐
    // │  #  │  S  │  T  │  P  │  H  │  *  │             │  *  │  F  │  P  │  L  │  T  │  D  │
    // ├─────┼─────┼─────┼─────┼─────┼─────┤             ├─────┼─────┼─────┼─────┼─────┼─────┤
    // │  #  │  S  │  K  │  W  │  R  │  *  │             │  *  │  R  │  B  │  G  │  S  │  Z  │
    // └─────┴─────┴─────┴─────┴─────┴─────┘             └─────┴─────┴─────┴─────┴─────┴─────┘
    //                         ┌───────────┐             ┌───────────┐
    //                         │     #     │             │     #     │
    //                         ├─────┬─────┤   ┌─────┐   ├─────┬─────┤
    //                         │  A  │  O  │   │MT_TG│   │  E  │  U  │
    //                         └─────┴─────┘   └─────┘   └─────┴─────┘
    // GEMINI
    [_GEMINI] = LAYOUT(
        STN_N1, STN_S1, STN_TL, STN_PL, STN_HL, STN_ST1, STN_ST3, STN_FR, STN_PR, STN_LR, STN_TR, STN_DR,
        STN_N2, STN_S2, STN_KL, STN_WL, STN_RL, STN_ST2, STN_ST4, STN_RR, STN_BR, STN_GR, STN_SR, STN_ZR,
                                        STN_N3, MT_TGL,  STN_N4,
                                        STN_A,  STN_O,   STN_E,   STN_U
    ),
    // NUMBER
    // ┌─────┬─────┬─────┬─────┬─────┬─────┐             ┌─────┬─────┬─────┬─────┬─────┬─────┐
    // │  `  │ 00  │  1  │  2  │  3  │  -  │             │ PGU │ HOM │  ↑  │ END │ CAP │ ALT │
    // ├─────┼─────┼──4──┼──5──┼──6──┼──,──┤             ├─────┼─────┼─────┼─────┼─────┼─────┤
    // │ ESC │  0  │  7  │  8  │  9  │  .  │             │ PGD │  ←  │  ↓  │  →  │ GUI │MO_FN│
    // └─────┴─────┴─────┴─────┴─────┴─────┘             └─────┴─────┴─────┴─────┴─────┴─────┘
    //                         ┌───────────┐             ┌───────────┐
    //                         │   SandS   │             │   EandS   │
    //                         ├─────┬─────┤   ┌─────┐   ├─────┬─────┤
    //                         │ ALT │ CTL │   │MT_TG│   │  [  =  ]  │
    //                         └─────┴─────┘   └─────┘   └─────┴─────┘
    // NUMBER Shifted
    // ┌─────┬─────┬─────┬─────┬─────┬─────┐             ┌─────┬─────┬─────┬─────┬─────┬─────┐
    // │  ~  │ ()← │  !  │  @  │  #  │  _  │             │ PGU │ HOM │  ↑  │ END │ CAP │ ALT │
    // ├─────┼─────┼──$──┼──%──┼──^──┼──<──┤             ├─────┼─────┼─────┼─────┼─────┼─────┤
    // │ ESC │  )  │  &  │  *  │  (  │  >  │             │ PGD │  ←  │  ↓  │  →  │ GUI │MO_FN│
    // └─────┴─────┴─────┴─────┴─────┴─────┘             └─────┴─────┴─────┴─────┴─────┴─────┘
    //                         ┌───────────┐             ┌───────────┐
    //                         │   SandS   │             │   EandS   │
    //                         ├─────┬─────┤   ┌─────┐   ├─────┬─────┤
    //                         │ ALT │ CTL │   │MT_TG│   │  {  +  }  │
    //                         └─────┴─────┘   └─────┘   └─────┴─────┘
    // NUMBER
    [_NUMBER] = LAYOUT(
        KC_GRV, KC_DZ,  KC_1, KC_2, KC_3,    KC_MINS,   KC_PGUP, KC_HOME, KC_UP,   KC_END,   KC_CAPS, TG_ALT,
        KC_ESC, KC_0,   KC_7, KC_8, KC_9,    KC_DOT,    KC_PGDN, KC_LEFT, KC_DOWN, KC_RIGHT, KC_LGUI, MO_FUN,
                                     MT_SPC,  KC_TRNS, MT_ENT,
                                     KC_LALT, KC_LCTL, KC_TRNS, KC_TRNS
    ),

    // FUNCTION
    // ┌─────┬─────┬─────┬─────┬─────┬─────┐             ┌─────┬─────┬─────┬─────┬─────┬─────┐
    // │  `  │ F1  │ F2  │ F3  │ F4  │ F5  │             │ BRU │ VL0 │ VL- │ VL+ │ PSC │ SBL │
    // ├─────┼─────┼─────┼─────┼─────┼─────┤             ├─────┼─────┼─────┼─────┼─────┼─────┤
    // │ ESC │ F6  │ F7  │ F8  │ F9  │ F10 │             │ BRD │ |<< │ >|| │ >>| │ INS │MO_FN│
    // └─────┴─────┴─────┴─────┴─────┴─────┘             └─────┴─────┴─────┴─────┴─────┴─────┘
    //                         ┌───────────┐             ┌───────────┐
    //                         │   SandS   │             │   EandS   │
    //                         ├─────┬─────┤   ┌─────┐   ├─────┬─────┤
    //                         │ ALT │ CTL │   │MT_TG│   │ F11 │ F12 │
    //                         └─────┴─────┘   └─────┘   └─────┴─────┘
    // FUNCTION
    [_FUNCTION] = LAYOUT(
        KC_GRV, KC_F1, KC_F2, KC_F3, KC_F4,   KC_F5,   KC_BRIU, KC_MUTE, KC_VOLD, KC_VOLU, KC_PSCR, TG_SBL,
        KC_ESC, KC_F6, KC_F7, KC_F8, KC_F9,   KC_F10,  KC_BRID, KC_MPRV, KC_MPLY, KC_MNXT, KC_LGUI, KC_TRNS,
                                     KC_TRNS, KC_TRNS, KC_TRNS,
                                     KC_TRNS, KC_TRNS, KC_F11,  KC_F12
    ),
};

/*---------------------------------------------------------------------------------------------------*/
/*--------------------------------------------Main Processing--------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/


bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    uint8_t mods = get_mods();
    bool shifted = (mods & MOD_MASK_SHIFT);

    if (record->event.pressed) {
        if (lshift_timer != 0 && !IS_MODIFIER_KEYCODE(keycode)) {
            lshift_has_key = true;
        }
        if (rshift_timer != 0 && !IS_MODIFIER_KEYCODE(keycode)) {
            rshift_has_key = true;
        }
    }

    if (is_combo_candidate(keycode)) {
        if (record->event.pressed) {
            if (combo_fifo_len < COMBO_FIFO_LEN) {
                uint16_t base = keycode;
                if (hold_state.is_held && combo_fifo_len > 0) {
                    unregister_code16(hold_state.keycode);
                    if (hold_state.shift_held) {
                        del_mods(MOD_LSFT);
                        send_keyboard_report();
                        hold_state.shift_held = false;
                    }
                    hold_state.is_held = false;
                }
                combo_fifo[combo_fifo_len].keycode = base;
                combo_fifo[combo_fifo_len].layer   = get_highest_layer(layer_state | default_layer_state);
                combo_fifo[combo_fifo_len].mods    = mods;
                combo_fifo[combo_fifo_len].time_pressed = timer_read();
                combo_fifo[combo_fifo_len].released = false;
                if ((mods & MOD_LSFT) && lshift_timer != 0) {
                    lshift_has_key = true;
                }
                if ((mods & MOD_RSFT) && rshift_timer != 0) {
                    rshift_has_key = true;
                }
                combo_fifo_len++;
            } else {
                uint8_t current_layer = get_highest_layer(layer_state | default_layer_state);
                transformed_key_t transformed = transform_key_extended(keycode, shifted, current_layer);
                if (transformed.needs_unshift) {
                    tap_code16_unshifted(transformed.keycode);
                } else {
                    tap_code16(transformed.keycode);
                }
            }
        } else {
            uint16_t base = keycode;
            if (hold_state.is_held) {
                if (base == hold_state.source_key_a) {
                    hold_state.source_a_pressed = false;
                }
                if (base == hold_state.source_key_b) {
                    hold_state.source_b_pressed = false;
                }
                if (!hold_state.source_a_pressed || !hold_state.source_b_pressed) {
                    unregister_code16(hold_state.keycode);
                    if (hold_state.shift_held) {
                        del_mods(MOD_LSFT);
                        send_keyboard_report();
                        hold_state.shift_held = false;
                    }
                    hold_state.is_held = false;
                    hold_state.keycode = 0;
                }
            }

            bool fifo_updated = false;
            for (uint8_t i = 0; i < combo_fifo_len; i++) {
                if (combo_fifo[i].keycode == base && !combo_fifo[i].released) {
                    combo_fifo[i].released = true;
                    fifo_updated = true;
                    break;
                }
            }
            if (fifo_updated) {
                combo_fifo_service_extended(transform_key_extended);
            }
        }
        return false;
    }

    switch (keycode) {
        case MT_TGL:
            if (record->tap.count > 0) {
                if (record->event.pressed) {
                    if (default_layer == 0) {
                        default_layer_set((layer_state_t)1UL << _GEMINI);
                        layer_move(_GEMINI);
                        default_layer = 1;
                    } else {
                        default_layer_set((layer_state_t)1UL << _QWERTY);
                        layer_move(_QWERTY);
                        default_layer = 0;
                    }
                }
                return false;
            }
            return true;
        case TG_ALT:
            return handle_toggle_on_hold(record, &tg_alt_state, toggle_alt_mode);
        case TG_SBL:
            return handle_toggle_on_hold(record, &tg_sbl_state, toggle_sbl_mode);
        case KC_DZ:
            if (record->event.pressed) {
                if (shifted) {
                    tap_code16(KC_LPRN);
                    tap_code16(KC_RPRN);
                    tap_code16_unshifted(KC_LEFT);
                } else {
                    dz_timer = timer_read();
                    dz_delayed = true;
                    dz_fifo_len_at_press = combo_fifo_len;
                }
            }
            return false;
        case KC_LSFT:
            if (record->event.pressed) {
                lshift_timer = timer_read();
                lshift_has_key = false;
            } else {
                bool lshift_in_fifo = false;
                for (uint8_t i = 0; i < combo_fifo_len; i++) {
                    if (combo_fifo[i].mods & MOD_LSFT) {
                        lshift_in_fifo = true;
                        break;
                    }
                }
                if (!lshift_has_key && !lshift_in_fifo && lshift_timer != 0 && timer_elapsed(lshift_timer) < COMBO_TIMEOUT_MS) {
                    tap_code16(KC_SPC);
                }
                lshift_timer = 0;
            }
            return true;
        case KC_RSFT:
            if (record->event.pressed) {
                rshift_timer = timer_read();
                rshift_has_key = false;
            } else {
                bool rshift_in_fifo = false;
                for (uint8_t i = 0; i < combo_fifo_len; i++) {
                    if (combo_fifo[i].mods & MOD_RSFT) {
                        rshift_in_fifo = true;
                        break;
                    }
                }
                if (!rshift_has_key && !rshift_in_fifo && rshift_timer != 0 && timer_elapsed(rshift_timer) < COMBO_TIMEOUT_MS) {
                    tap_code16(KC_ENT);
                }
                rshift_timer = 0;
            }
            return true;
        case KC_LCTL:
            if (is_mac) {
                if (record->event.pressed) {
                    register_mods(MOD_LGUI);
                } else {
                    unregister_mods(MOD_LGUI);
                }
                return false;
            }
            return true;
        case KC_LGUI:
            if (is_mac) {
                if (record->event.pressed) {
                    register_mods(MOD_LCTL);
                } else {
                    unregister_mods(MOD_LCTL);
                }
                return false;
            }
            return true;
        case KC_RCTL:
            if (is_mac) {
                if (record->event.pressed) {
                    register_mods(MOD_RGUI);
                } else {
                    unregister_mods(MOD_RGUI);
                }
                return false;
            }
            return true;
        case KC_RGUI:
            if (is_mac) {
                if (record->event.pressed) {
                    register_mods(MOD_RCTL);
                } else {
                    unregister_mods(MOD_RCTL);
                }
                return false;
            }
            return true;
        default:
            break;
    }
    return true;
}

void matrix_scan_user(void) {
    if (!os_detected) {
        os_variant_t os = detected_host_os();
        if (os == OS_MACOS || os == OS_IOS) {
            is_mac = true;
            os_detected = true;
        }
    }
    refresh_force_qwerty_state();
    combo_fifo_service_extended(transform_key_extended);

    if (dz_delayed) {
        if (dz_fifo_len_at_press == 0) {
            SEND_STRING("00");
            dz_delayed = false;
        } else if (combo_fifo_len < dz_fifo_len_at_press) {
            SEND_STRING("00");
            dz_delayed = false;
        } else if (timer_elapsed(dz_timer) >= COMBO_TIMEOUT_MS) {
            SEND_STRING("00");
            dz_delayed = false;
        }
    }
}
