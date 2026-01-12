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

enum layer_names {
    _QWERTY = 0,
    _GEMINI,
    _NUMBER,
    _FUNCTION,
};

enum custom_keycodes {
    KC_DZ = SAFE_RANGE,  // 00キー
    TG_ALT,              // Alternative Layout切替キー
};

#define MT_SPC MT(MOD_LSFT, KC_SPC)  // タップでSpace、ホールドでShift
#define MT_ENT MT(MOD_LSFT, KC_ENT)  // タップでEnter、ホールドでShift
#define MT_ESC MT(MOD_LGUI, KC_ESC)  // タップでEscape、ホールドでGUI
#define MO_FUN MO(_FUNCTION)  // ホールドで_FUNCTIONレイヤー
#define MT_TGL LT(_NUMBER, KC_F24)  // タップで_QWERTY・_GEMINIレイヤー切替、ホールドで_NUMBERレイヤー

static uint16_t default_layer = 0; // (0:_QWERTY, 1: _GEMINI)
static bool is_alt_mode = false;
static bool force_qwerty_active = false;
static bool is_mac = false;
static bool os_detected = false;  // OS検知完了フラグ

typedef union {
    uint32_t raw;
    struct {
        bool alt_mode : 1;
    };
} user_config_t;

static user_config_t user_config;

void eeconfig_init_user(void) {
    user_config.raw = 0;
    user_config.alt_mode = false;
    eeconfig_update_user(user_config.raw);
    steno_set_mode(STENO_MODE_GEMINI);
}

void keyboard_post_init_user(void) {
    os_variant_t os = detected_host_os();
    is_mac = (os == OS_MACOS || os == OS_IOS);
    user_config.raw = eeconfig_read_user();
    is_alt_mode = (user_config.alt_mode);
    default_layer_set((layer_state_t)1UL << _QWERTY);
    layer_clear();
    layer_move(_QWERTY);
    default_layer = 0;
}

/*---------------------------------------------------------------------------------------------------*/
/*----------------------------------------Alternative Layout-----------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/

// Alternative Layout変換
// 配列名：QWERTY
static inline uint16_t alt_transform(uint16_t kc) {
    if (!is_alt_mode || force_qwerty_active) return kc;
    switch (kc) {
        case KC_Q: return KC_Q;
        case KC_W: return KC_W;
        case KC_E: return KC_E;
        case KC_R: return KC_R;
        case KC_T: return KC_T;
        case KC_Y: return KC_Y;
        case KC_U: return KC_U;
        case KC_I: return KC_I;
        case KC_O: return KC_O;
        case KC_P: return KC_P;
        case KC_MINS: return KC_MINS;

        case KC_A: return KC_A;
        case KC_S: return KC_S;
        case KC_D: return KC_D;
        case KC_F: return KC_F;
        case KC_G: return KC_G;
        case KC_H: return KC_H;
        case KC_J: return KC_J;
        case KC_K: return KC_K;
        case KC_L: return KC_L;
        case KC_SCLN: return KC_SCLN;
        case KC_QUOT: return KC_QUOT;

        case KC_Z: return KC_Z;
        case KC_X: return KC_X;
        case KC_C: return KC_C;
        case KC_V: return KC_V;
        case KC_B: return KC_B;
        case KC_N: return KC_N;
        case KC_M: return KC_M;
        case KC_COMM: return KC_COMM;
        case KC_DOT: return KC_DOT;
        case KC_SLSH: return KC_SLSH;
        case KC_BSLS: return KC_BSLS;
        default: return kc;
    }
}



static inline bool mods_except_shift_active(void) {
    uint8_t mods = get_mods() | get_weak_mods() | get_oneshot_mods();
    mods &= ~MOD_MASK_SHIFT;
    return mods != 0;
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

/*---------------------------------------------------------------------------------------------------*/
/*--------------------------------------------FIFOコンボ----------------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/

#define COMBO_FIFO_LEN       30  // FIFOの長さ
#define COMBO_TIMEOUT_MS     300 // コンボ待機のタイムアウト時間(ms) ※ QMKコンボでいうところのCOMBO_TERM

typedef struct {
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
    uint16_t source_key_a;
    uint16_t source_key_b;
    bool     source_a_pressed;
    bool     source_b_pressed;
} hold_state_t;

// コンボ定義（順不同）
static const combo_pair_t combo_pairs[] PROGMEM = {
    // QWERTY
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

    // NUMBER
    {KC_1,    KC_7,    KC_4,     _NUMBER},
    {KC_2,    KC_8,    KC_5,     _NUMBER},
    {KC_3,    KC_9,    KC_6,     _NUMBER},
    {KC_DOT,  KC_MINS, KC_COMMA, _NUMBER},
    {KC_9,    KC_0,    KC_TAB,   _NUMBER},
    {KC_3,    KC_DZ,   KC_ESC,   _NUMBER},
    {KC_PGDN, KC_LEFT, KC_BSPC,  _NUMBER},
    {KC_PGUP, KC_HOME, KC_DEL,   _NUMBER},
    {KC_LBRC, KC_RBRC, KC_EQL,   _NUMBER},
};
#define COMBO_PAIR_COUNT (sizeof(combo_pairs) / sizeof(combo_pairs[0]))

static combo_event_t combo_fifo[COMBO_FIFO_LEN];
static uint8_t combo_fifo_len = 0;
static hold_state_t hold_state = {0, 0, false, 0, 0, false, false};

// コンボ候補キーか判定する関数
static bool is_combo_candidate(uint16_t keycode) {

    if (keycode == KC_DZ) return false;
    // モディファイア類はコンボから除外（入れ替え処理や修飾の素通しのため）
    switch (keycode) {
        case KC_LCTL: case KC_RCTL:
        case KC_LGUI: case KC_RGUI:
        case KC_LALT: case KC_RALT:
        case KC_LSFT: case KC_RSFT:
            return false;
    }

    uint16_t base = keycode;
        for (uint8_t i = 0; i < COMBO_PAIR_COUNT; i++) {
        combo_pair_t pair;
        memcpy_P(&pair, &combo_pairs[i], sizeof(pair));
        if (pair.a == base || pair.b == base) {
            return true;
        }
    }
    return false;
}

// コンボ定義を検索する関数
static const combo_pair_t *find_combo(uint16_t a, uint16_t b, uint8_t layer) {
    for (uint8_t i = 0; i < COMBO_PAIR_COUNT; i++) {
        combo_pair_t pair;
        memcpy_P(&pair, &combo_pairs[i], sizeof(pair));
        if (pair.layer != layer) continue;
        if ((pair.a == a && pair.b == b) || (pair.a == b && pair.b == a)) {
            return &combo_pairs[i];
        }
    }
    return NULL;
}

// 指定インデックスの要素を削除する関数
static void fifo_remove(uint8_t idx) {
    if (idx >= combo_fifo_len) return;
    for (uint8_t i = idx; i + 1 < combo_fifo_len; i++) {
        combo_fifo[i] = combo_fifo[i + 1];
    }
    combo_fifo_len--;
}

static void clear_hold_state(void) {
    if (hold_state.is_held) {
        unregister_code16(hold_state.keycode);
        hold_state.is_held = false;
        hold_state.keycode = 0;
    }
}

// 先頭要素とそれ以外のペアを探索し、コンボがあれば発射して削除
static bool resolve_combo_head(void) {
    if (combo_fifo_len < 2) return false;

    uint16_t head_kc    = combo_fifo[0].keycode;
    uint8_t  head_layer = combo_fifo[0].layer;

    for (uint8_t i = 1; i < combo_fifo_len; i++) {
        uint16_t other_kc    = combo_fifo[i].keycode;
        uint8_t  other_layer = combo_fifo[i].layer;

        if (other_layer != head_layer) continue;

        const combo_pair_t *hit = find_combo(head_kc, other_kc, head_layer);
        if (hit) {
            combo_pair_t pair;
            memcpy_P(&pair, hit, sizeof(pair));

            uint16_t out_kc = alt_transform(pair.out);
            bool head_pressed  = !combo_fifo[0].released;
            bool other_pressed = !combo_fifo[i].released;

            // If either source is already released, treat as a tap to avoid stuck holds.
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

// タイムアウト処理とコンボ解決を行う
static void combo_fifo_service(void) {
    while (combo_fifo_len > 0) {
        // FIFOに1つのキーしかない場合
        if (combo_fifo_len == 1) {
            // キーが離されている場合、即座に出力
            if (combo_fifo[0].released) {
                uint16_t base_kc = combo_fifo[0].keycode;
                uint16_t out_kc = alt_transform(base_kc);
                tap_code16(out_kc);
                fifo_remove(0);
                continue;
            }
            // キーがまだ押されていてタイムアウト時間経過の場合
            if (timer_elapsed(combo_fifo[0].time_pressed) > COMBO_TIMEOUT_MS) {
                uint16_t base_kc = combo_fifo[0].keycode;
                uint16_t out_kc = alt_transform(base_kc);
                clear_hold_state();
                hold_state.keycode = out_kc;
                hold_state.time_confirmed = timer_read();
                hold_state.is_held = true;
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
        // FIFOに2つ以上のキーがある場合
        if (combo_fifo_len >= 2) {
            if (resolve_combo_head()) {
                continue;
            }
            // コンボが成立しなかった場合、先頭要素の離脱状態を確認
            if (combo_fifo[0].released) {
                // コンボ候補のキーが離されている場合、即座に出力
                if (hold_state.is_held) {
                    unregister_code16(hold_state.keycode);
                    hold_state.is_held = false;
                }
                uint16_t base_kc = combo_fifo[0].keycode;
                uint16_t out_kc = alt_transform(base_kc);
                fifo_remove(0);
                tap_code16(out_kc);
                continue;
            } else {
                break;
            }
        }
        break;
    }
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
    // │  `  │  -  │  1  │  2  │  3  │ 00  │             │ PGU │ HOM │  ↑  │ END │ CAP │ ALT │
    // ├─────┼──,──┼──4──┼──5──┼──6──┼─────┤             ├─────┼─────┼─────┼─────┼─────┼─────┤
    // │ ESC │  .  │  7  │  8  │  9  │  0  │             │ PGD │  ←  │  ↓  │  →  │ GUI │MO_FN│
    // └─────┴─────┴─────┴─────┴─────┴─────┘             └─────┴─────┴─────┴─────┴─────┴─────┘
    //                         ┌───────────┐             ┌───────────┐
    //                         │   SandS   │             │   EandS   │
    //                         ├─────┬─────┤   ┌─────┐   ├─────┬─────┤
    //                         │ ALT │ CTL │   │MT_TG│   │INT5 │INT4 │
    //                         └─────┴─────┘   └─────┘   └─────┴─────┘
    // NUMBER
    [_NUMBER] = LAYOUT(
        KC_GRV, KC_MINS, KC_1, KC_2, KC_3,    KC_DZ,   KC_PGUP, KC_HOME, KC_UP,   KC_END,   KC_CAPS, TG_ALT,
        KC_ESC, KC_DOT,  KC_7, KC_8, KC_9,    KC_0,    KC_PGDN, KC_LEFT, KC_DOWN, KC_RIGHT, KC_LGUI, MO_FUN,
                                     MT_SPC,  KC_TRNS, MT_ENT,
                                     KC_LALT, KC_LCTL, KC_LBRC, KC_RBRC
    ),

    // FUNCTION
    // ┌─────┬─────┬─────┬─────┬─────┬─────┐             ┌─────┬─────┬─────┬─────┬─────┬─────┐
    // │ F1  │ F2  │ F3  │ F4  │ F5  │ F11 │             │ BRU │ VL0 │ VL- │ VL+ │ PSC │ xxx │
    // ├─────┼─────┼─────┼─────┼─────┼─────┤             ├─────┼─────┼─────┼─────┼─────┼─────┤
    // │ ESC │ F6  │ F7  │ F8  │ F9  │ F10 │             │ BRD │ |<< │ >|| │ >>| │ INS │MO_FN│
    // └─────┴─────┴─────┴─────┴─────┴─────┘             └─────┴─────┴─────┴─────┴─────┴─────┘
    //                         ┌───────────┐             ┌───────────┐
    //                         │   SandS   │             │   EandS   │
    //                         ├─────┬─────┤   ┌─────┐   ├─────┬─────┤
    //                         │ ALT │ CTL │   │MT_TG│   │ F12 │ F13 │
    //                         └─────┴─────┘   └─────┘   └─────┴─────┘
    // FUNCTION
    [_FUNCTION] = LAYOUT(
        KC_F1,  KC_F2, KC_F3, KC_F4, KC_F5,   KC_F11,  KC_BRIU, KC_MUTE, KC_VOLD, KC_VOLU, KC_PSCR, KC_NO,
        KC_ESC, KC_F6, KC_F7, KC_F8, KC_F9,   KC_F10,  KC_BRID, KC_MPRV, KC_MPLY, KC_MNXT, KC_LGUI, KC_TRNS,
                                     KC_TRNS, KC_TRNS, KC_TRNS,
                                     KC_TRNS, KC_TRNS, KC_F12,  KC_F13
    ),
};

/*---------------------------------------------------------------------------------------------------*/
/*--------------------------------------------メイン処理----------------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/


bool process_record_user(uint16_t keycode, keyrecord_t *record) {

    if (is_combo_candidate(keycode)) {

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
                combo_fifo[combo_fifo_len].released = false;  // 初期化
                combo_fifo_len++;
            } else {
                uint16_t out_kc = alt_transform(keycode);
                tap_code16(out_kc);
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
                combo_fifo_service();
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
            if (record->event.pressed) {
                is_alt_mode = !is_alt_mode;
                user_config.alt_mode = is_alt_mode;
                eeconfig_update_user(user_config.raw);
            }
            return true;
        case KC_DZ:
            if (record->event.pressed) {
                SEND_STRING("00");
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
    combo_fifo_service();
}
