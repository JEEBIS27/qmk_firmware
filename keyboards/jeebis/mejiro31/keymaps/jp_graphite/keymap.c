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
/*----------------------------------------------初期設定----------------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/

#include QMK_KEYBOARD_H
#include "os_detection.h"
#include "keymap_japanese.h"
#include "jis_transform.h"
#include "combo_fifo.h"
#include "alt_layout.h"

enum layer_names {
    _QWERTY = 0,
    _GEMINI,
    _NUMBER,
    _FUNCTION,
};

enum custom_keycodes {
    KC_DZ = SAFE_RANGE,  // 00キー
    KC_TZ,               // 000キー
    TG_JIS,              // JISモード切替キー
    TG_ALT,              // Alternative Layout切替キー
    TG_SBL,              // Mejiro31独自記号レイアウト切替キー
};

#define MT_SPC MT(MOD_LSFT, KC_SPC)
#define MT_ENT MT(MOD_LSFT, KC_ENT)
#define MT_ESC MT(MOD_LGUI, KC_ESC)
#define MO_FUN MO(_FUNCTION)
#define MT_TGL LT(_NUMBER, KC_F24)

bool is_jis_mode = true;
bool is_sbl_mode = true;
static bool is_mac = false;
static bool os_detected = false;
static uint16_t dz_timer = 0;
static bool dz_delayed = false;
// 0:未使用, 1:英語, 2:日本語, 3:無変更
static int stn_lang = 2; // ステノ時の言語
static int kbd_lang = 1; // キーボード時の言語
static int alt_lang = 1; // Alternative Layoutの言語設定


typedef union {
    uint32_t raw;
    struct {
        bool jis_mode : 1;
        bool alt_mode : 1;
        bool sbl_mode : 1;
    };
} user_config_t;

static user_config_t user_config;

void eeconfig_init_user(void) {
    user_config.raw = 0;
    user_config.jis_mode = true;
    user_config.alt_mode = true;
    user_config.sbl_mode = true;
    eeconfig_update_user(user_config.raw);
    steno_set_mode(STENO_MODE_GEMINI);
}

void keyboard_post_init_user(void) {
    os_variant_t os = detected_host_os();
    is_mac = (os == OS_MACOS || os == OS_IOS);
    user_config.raw = eeconfig_read_user();
    is_jis_mode = (user_config.jis_mode);
    is_alt_mode = (user_config.alt_mode);
    is_sbl_mode = (user_config.sbl_mode);
    default_layer_set((layer_state_t)1UL << _QWERTY);
    layer_clear();
    layer_move(_QWERTY);
    default_layer = 0;
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
    //                         │ ALT │ CTL │   │MT_TG│   │INT5 │INT4 │
    //                         └─────┴─────┘   └─────┘   └─────┴─────┘
    // NUMBER Shifted
    // ┌─────┬─────┬─────┬─────┬─────┬─────┐             ┌─────┬─────┬─────┬─────┬─────┬─────┐
    // │  ~  │  %  │  [  │  {  │  (  │  <  │             │ PGU │ HOM │  ↑  │ END │ CAP │ ALT │
    // ├─────┼──/──┼──*──┼──=──┼──+──┼──^──┤             ├─────┼─────┼─────┼─────┼─────┼─────┤
    // │ ESC │  $  │  ]  │  }  │  )  │  >  │             │ PGD │  ←  │  ↓  │  →  │ GUI │MO_FN│
    // └─────┴─────┴─────┴─────┴─────┴─────┘             └─────┴─────┴─────┴─────┴─────┴─────┘
    //                         ┌───────────┐             ┌───────────┐
    //                         │   SandS   │             │   EandS   │
    //                         ├─────┬─────┤   ┌─────┐   ├─────┬─────┤
    //                         │ ALT │ CTL │   │MT_TG│   │INT5 │INT4 │
    //                         └─────┴─────┘   └─────┘   └─────┴─────┘

    {KC_LBRC, KC_EXLM, KC_AMPR,    _QWERTY},  // [ / ! / &
    {KC_RBRC, KC_QUES, KC_PIPE,    _QWERTY},  // ] / ? / |
    {KC_EQL,  KC_HASH, KC_AT,      _QWERTY},  // = / # / @

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

uint16_t sbl_transform(uint16_t kc, bool shifted) {
    if (!is_sbl_mode || force_qwerty_active) return kc;

    uint8_t current_layer = get_highest_layer(layer_state | default_layer_state);

    for (uint8_t i = 0; i < sizeof(sbl_mappings) / sizeof(sbl_mappings[0]); i++) {
        sbl_mapping_t mapping;
        memcpy_P(&mapping, &sbl_mappings[i], sizeof(mapping));
        if (mapping.layer != current_layer) continue;
        if (mapping.base == kc) {
            return shifted ? mapping.shifted : mapping.unshifted;
        }
    }

    return kc;
}

/*---------------------------------------------------------------------------------------------------*/
/*----------------------------------------Alternative Layout-----------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/

// 配列名：Graphite
static const alt_mapping_t alt_mappings[] PROGMEM = {
    {KC_Q,    KC_B,    KC_B},
    {KC_W,    KC_L,    KC_L},
    {KC_E,    KC_D,    KC_D},
    {KC_R,    KC_W,    KC_W},
    {KC_T,    KC_Z,    KC_Z},
    {KC_Y,    KC_QUOT, KC_UNDS},  // ' / _
    {KC_U,    KC_F,    KC_F},
    {KC_I,    KC_O,    KC_O},
    {KC_O,    KC_U,    KC_U},
    {KC_P,    KC_J,    KC_J},
    {KC_MINS, KC_SCLN, KC_SCLN},  // ; / :

    {KC_A,    KC_N,    KC_N},
    {KC_S,    KC_R,    KC_R},
    {KC_D,    KC_T,    KC_T},
    {KC_F,    KC_S,    KC_S},
    {KC_G,    KC_G,    KC_G},
    {KC_H,    KC_Y,    KC_Y},
    {KC_J,    KC_H,    KC_H},
    {KC_K,    KC_A,    KC_A},
    {KC_L,    KC_E,    KC_E},
    {KC_SCLN, KC_I,    KC_I},
    {KC_QUOT, KC_COMM, KC_QUES},  // , / ?

    {KC_Z,    KC_Q,    KC_Q},
    {KC_X,    KC_X,    KC_X},
    {KC_C,    KC_M,    KC_M},
    {KC_V,    KC_C,    KC_C},
    {KC_B,    KC_V,    KC_V},
    {KC_N,    KC_K,    KC_K},
    {KC_M,    KC_P,    KC_P},
    {KC_COMM, KC_DOT,  KC_RABK},   // . / >
    {KC_DOT,  KC_MINS, KC_DQUO},  // - / "
    {KC_SLSH, KC_SLSH, KC_LABK},  // / / <
    {KC_BSLS, KC_BSLS, KC_BSLS},
};

uint16_t alt_transform(uint16_t kc, bool shifted) {
    if (!is_alt_mode || force_qwerty_active) return kc;

    uint8_t current_layer = get_highest_layer(layer_state | default_layer_state);
    if (current_layer != _QWERTY) return kc;

    for (uint8_t i = 0; i < sizeof(alt_mappings) / sizeof(alt_mappings[0]); i++) {
        alt_mapping_t mapping;
        memcpy_P(&mapping, &alt_mappings[i], sizeof(mapping));
        if (mapping.base == kc) {
            return shifted ? mapping.shifted : mapping.unshifted;
        }
    }

    return kc;
}

static inline transformed_key_t transform_key_extended(uint16_t kc, bool shifted) {
    uint16_t sbl_kc = sbl_transform(kc, shifted);
    uint16_t alt_kc = alt_transform(sbl_kc, shifted);

    // Check if shift should be removed for SBL or JIS
    bool needs_unshift = is_jis_shift_target(alt_kc, shifted);

    // Also check SBL: when shifted and sbl_transform changed the key
    if (!needs_unshift && shifted && is_sbl_mode && !force_qwerty_active) {
        uint8_t current_layer = get_highest_layer(layer_state | default_layer_state);
        if (current_layer == _NUMBER || current_layer == _QWERTY) {
            if (sbl_kc != kc) {
                needs_unshift = true;
            }
        }
    }

    transformed_key_t result = {
        .keycode = jis_transform(alt_kc, shifted),
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

// lang : 0=なし, 1=英語, 2=日本語
static void update_lang(uint8_t lang) {
    switch (alt_lang) {
        case 0:
            is_alt_mode = false;
            break;
        case 1:
            is_alt_mode = (lang == 1);
            break;
        case 2:
            is_alt_mode = (lang == 2);
            break;
        default:
            break;
    }
    switch (lang) {
        case 1:
            tap_code16(is_mac ? KC_LNG2 : KC_INT5); // Mac: 英数 / Win: 無変換
            break;
        case 2:
            tap_code16(is_mac ? KC_LNG1 : KC_INT4); // Mac: かな / Win: 変換
            break;
        default:
            break;
    }
    user_config.alt_mode = is_alt_mode;
    eeconfig_update_user(user_config.raw);
}

/*---------------------------------------------------------------------------------------------------*/
/*--------------------------------------------FIFOコンボ----------------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/

// コンボ定義（順不同）
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
    if (keycode == KC_INT4) return false;
    if (keycode == KC_INT5) return false;
    if (keycode == KC_GRV) return true;
    if (keycode == KC_DZ) return is_sbl_mode && shifted;
    if (keycode == KC_TZ) return is_sbl_mode && shifted;
    return is_combo_candidate_default(keycode, 0);
}

/*---------------------------------------------------------------------------------------------------*/
/*--------------------------------------------キーマップ----------------------------------------------*/
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
    // │  `  │ 00  │  1  │  2  │  3 ESC -  │             │ PGU │ HOM │  ↑  │ END │ CAP │ ALT │
    // ├─────┼─────┼──4──┼──5──┼──6──┼──,──┤             ├─────┼─────┼─────┼─────┼─────┼─────┤
    // │ ESC │  0  │  7  │  8  │  9 TAB .  │             │ PGD │  ←  │  ↓  │  →  │ GUI │MO_FN│
    // └─────┴─────┴─────┴─────┴─────┴─────┘             └─────┴─────┴─────┴─────┴─────┴─────┘
    //                         ┌───────────┐             ┌───────────┐
    //                         │   SandS   │             │   EandS   │
    //                         ├─────┬─────┤   ┌─────┐   ├─────┬─────┤
    //                         │ ALT │ CTL │   │MT_TG│   │INT5 │INT4 │
    //                         └─────┴─────┘   └─────┘   └─────┴─────┘
    // NUMBER Shifted
    // ┌─────┬─────┬─────┬─────┬─────┬─────┐             ┌─────┬─────┬─────┬─────┬─────┬─────┐
    // │  ~  │ ()← │  !  │  @  │  # ESC _  │             │ PGU │ HOM │  ↑  │ END │ CAP │ ALT │
    // ├─────┼─────┼──$──┼──%──┼──^──┼──<──┤             ├─────┼─────┼─────┼─────┼─────┼─────┤
    // │ ESC │  )  │  &  │  *  │  ( TAB >  │             │ PGD │  ←  │  ↓  │  →  │ GUI │MO_FN│
    // └─────┴─────┴─────┴─────┴─────┴─────┘             └─────┴─────┴─────┴─────┴─────┴─────┘
    //                         ┌───────────┐             ┌───────────┐
    //                         │   SandS   │             │   EandS   │
    //                         ├─────┬─────┤   ┌─────┐   ├─────┬─────┤
    //                         │ ALT │ CTL │   │MT_TG│   │INT5 │INT4 │
    //                         └─────┴─────┘   └─────┘   └─────┴─────┘
    // NUMBER
    [_NUMBER] = LAYOUT(
        KC_GRV, KC_DZ,  KC_1, KC_2, KC_3,    KC_MINS,   KC_PGUP, KC_HOME, KC_UP,   KC_END,   KC_CAPS, TG_ALT,
        KC_ESC, KC_0,   KC_7, KC_8, KC_9,    KC_DOT,    KC_PGDN, KC_LEFT, KC_DOWN, KC_RIGHT, KC_LGUI, MO_FUN,
                                     MT_SPC,  KC_TRNS, MT_ENT,
                                     KC_LALT, KC_LCTL, KC_INT5, KC_INT4
    ),

    // FUNCTION
    // ┌─────┬─────┬─────┬─────┬─────┬─────┐             ┌─────┬─────┬─────┬─────┬─────┬─────┐
    // │ JIS │ F1  │ F2  │ F3  │ F4  │ F5  │             │ BRU │ VL0 │ VL- │ VL+ │ PSC │ SBL │
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
        TG_JIS, KC_F1, KC_F2, KC_F3, KC_F4,   KC_F5,   KC_BRIU, KC_MUTE, KC_VOLD, KC_VOLU, KC_PSCR, TG_SBL,
        KC_ESC, KC_F6, KC_F7, KC_F8, KC_F9,   KC_F10,  KC_BRID, KC_MPRV, KC_MPLY, KC_MNXT, KC_LGUI, KC_TRNS,
                                     KC_TRNS, KC_TRNS, KC_TRNS,
                                     KC_TRNS, KC_TRNS, KC_F11,  KC_F12
    ),
};

/*---------------------------------------------------------------------------------------------------*/
/*--------------------------------------------メイン処理----------------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/


bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    uint8_t mods = get_mods();
    bool shifted = (mods & MOD_MASK_SHIFT);

    if (is_combo_candidate(keycode)) {
        if (keycode == KC_GRV) {
            if (is_jis_mode && (mods & MOD_MASK_ALT)) {
                if (record->event.pressed) {
                    tap_code16(KC_GRV); // 全角半角
                }
                return false;
            }
        }

        if (record->event.pressed) {
            if (combo_fifo_len < COMBO_FIFO_LEN) {
                uint16_t base = keycode;
                if (hold_state.is_held && combo_fifo_len > 0) {
                    unregister_code16(hold_state.keycode);
                    hold_state.is_held = false;
                }
                combo_fifo[combo_fifo_len].keycode = base;
                combo_fifo[combo_fifo_len].layer   = get_highest_layer(layer_state | default_layer_state);
                combo_fifo[combo_fifo_len].time_pressed = timer_read();
                combo_fifo[combo_fifo_len].released = false;
                combo_fifo_len++;
            } else {
                transformed_key_t transformed = transform_key_extended(keycode, shifted);
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
                        update_lang(stn_lang);
                    } else {
                        default_layer_set((layer_state_t)1UL << _QWERTY);
                        layer_move(_QWERTY);
                        default_layer = 0;
                        update_lang(kbd_lang);
                    }
                }
                return false;
            }
            return true;
        case TG_JIS:
            if (record->event.pressed) {
                is_jis_mode = !is_jis_mode;
                user_config.jis_mode = is_jis_mode;
                eeconfig_update_user(user_config.raw);
            }
            return true;
        case TG_ALT:
            if (record->event.pressed) {
                is_alt_mode = !is_alt_mode;
                user_config.alt_mode = is_alt_mode;
                eeconfig_update_user(user_config.raw);
            }
            return true;
        case TG_SBL:
            if (record->event.pressed) {
                is_sbl_mode = !is_sbl_mode;
                user_config.sbl_mode = is_sbl_mode;
                eeconfig_update_user(user_config.raw);
            }
            return true;
        case KC_DZ:
            if (record->event.pressed) {
                if (shifted) {
                    tap_code16(is_jis_mode ? JP_LPRN : KC_LPRN);
                    tap_code16(is_jis_mode ? JP_RPRN : KC_RPRN);
                    tap_code16_unshifted(KC_LEFT);
                } else {
                    dz_timer = timer_read();
                    dz_delayed = true;
                }
            }
            return false;
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
        case KC_INT4:
            update_lang(2); // 日本語へ切り替え
            if (stn_lang == 2) {
                default_layer = 1;
            }
            return false;
        case KC_INT5:
            update_lang(1); // 英語へ切り替え
            if (stn_lang == 1) {
                default_layer = 0;
            }
            return false;
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

    if (dz_delayed && timer_elapsed(dz_timer) >= 200) {
        SEND_STRING("00");
        dz_delayed = false;
    }
}
