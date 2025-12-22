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

#include QMK_KEYBOARD_H
#include "os_detection.h"
#include "keymap_japanese.h"

// レイヤー定義（enumの値を0から連番で確保する）
enum layer_names {
    _QWERTY = 0,
    _GEMINI,
    _NUMBER,
    _FUNCTION,
};

enum custom_keycodes {
    KC_DZ = SAFE_RANGE,  // 00キー
};

// shift+2  " -> @
const key_override_t kor_at = ko_make_with_layers(MOD_MASK_SHIFT, KC_2, JP_AT, 1);
// shift+6  & -> ^
const key_override_t kor_circ = ko_make_with_layers(MOD_MASK_SHIFT, KC_6, JP_CIRC, 1);
// shift+7  ' -> &
const key_override_t kor_ampr = ko_make_with_layers(MOD_MASK_SHIFT, KC_7, JP_AMPR, 1);
// shift+8  ( -> *
const key_override_t kor_astr = ko_make_with_layers(MOD_MASK_SHIFT, KC_8, JP_ASTR, 1);
// shift+9  ) -> (
const key_override_t kor_lprn = ko_make_with_layers(MOD_MASK_SHIFT, KC_9, JP_LPRN, 1);
// shift+0    -> )
const key_override_t kor_rprn = ko_make_with_layers(MOD_MASK_SHIFT, KC_0, JP_RPRN, 1);
// shift+-  = -> _
const key_override_t kor_unds = ko_make_with_layers(MOD_MASK_SHIFT, KC_MINS, JP_UNDS, 1);
// =        ^ -> =
// shift+=  ~ -> +
const key_override_t kor_eql = ko_make_with_layers_and_negmods(0, JP_CIRC, JP_EQL, 1, MOD_MASK_SHIFT);
const key_override_t kor_plus = ko_make_with_layers(MOD_MASK_SHIFT, JP_CIRC, JP_PLUS, 1);
/* \        ] -> \ */
/* shift+\  } -> | */
const key_override_t kor_bsls = ko_make_with_layers_and_negmods(0, KC_BSLS, JP_BSLS, 1, MOD_MASK_SHIFT);
const key_override_t kor_pipe = ko_make_with_layers(MOD_MASK_SHIFT, KC_BSLS, JP_PIPE, 1);
// [        @ -> [
// shift+[  ` -> {
const key_override_t kor_lbrc = ko_make_with_layers_and_negmods(0, JP_AT, JP_LBRC, 1, MOD_MASK_SHIFT);
const key_override_t kor_lcbr = ko_make_with_layers(MOD_MASK_SHIFT, JP_AT, JP_LCBR, 1);
// ]        [ -> ]
// shift+]  { -> }
const key_override_t kor_rbrc = ko_make_with_layers_and_negmods(0, JP_LBRC, JP_RBRC, 1, MOD_MASK_SHIFT);
const key_override_t kor_rcbr = ko_make_with_layers(MOD_MASK_SHIFT, JP_LBRC, JP_RCBR, 1);
// shift+;  + -> :
const key_override_t kor_coln = ko_make_with_layers(MOD_MASK_SHIFT, KC_SCLN, JP_COLN, 1);
// '        : -> '
// shift+'  * -> "
const key_override_t kor_quot = ko_make_with_layers_and_negmods(0, KC_QUOT, JP_QUOT, 1, MOD_MASK_SHIFT);
const key_override_t kor_dquo = ko_make_with_layers(MOD_MASK_SHIFT, KC_QUOT, JP_DQUO, 1);
// `        全角半角 -> `
// shift+`  shift+全角半角 -> ~
const key_override_t kor_grv = ko_make_with_layers_and_negmods(0, JP_ZKHK, JP_GRV, 1, MOD_MASK_SHIFT);
const key_override_t kor_tild = ko_make_with_layers(MOD_MASK_SHIFT, JP_ZKHK, JP_TILD, 1);
// Caps     英数 -> Caps
const key_override_t kor_caps = ko_make_with_layers_and_negmods(0, JP_EISU, JP_CAPS, 1, MOD_MASK_SHIFT);

#define MT_SPC MT(MOD_LSFT, KC_SPC)  // タップでSpace、ホールドでShift
#define MT_ENT MT(MOD_LSFT, KC_ENT)  // タップでEnter、ホールドでShift
#define MT_ESC MT(MOD_LGUI, KC_ESC)  // タップでEscape、ホールドでGUI
#define MO_FUN MO(_FUNCTION)  // ホールドで_FUNCTIONレイヤー
#define MT_TGL LT(_NUMBER, KC_F24)  // タップで_GEMINIレイヤー切替、ホールドで_NUMBERレイヤー

#define TG_JIS QK_KEY_OVERRIDE_TOGGLE // JISモード切替キー

static uint16_t default_layer = 0; // デフォルトレイヤー状態を保存する変数 (0:_QWERTY, 1: _GEMINI)
static bool is_jis_mode = true;    // JISモード状態を保存する変数

void eeconfig_init_user(void) {
    steno_set_mode(STENO_MODE_GEMINI);
}

const key_override_t *key_overrides[] = {
    &kor_at,
    &kor_circ,
    &kor_ampr,
    &kor_astr,
    &kor_lprn,
    &kor_rprn,
    &kor_unds,
    &kor_eql,
    &kor_plus,
    &kor_bsls,
    &kor_pipe,
    &kor_lbrc,
    &kor_lcbr,
    &kor_rbrc,
    &kor_rcbr,
    &kor_coln,
    &kor_quot,
    &kor_dquo,
    &kor_grv,
    &kor_tild,
    &kor_caps,
    NULL
};

/* -------------------------------------------------------------------------- */
/*  FIFO-based combo algorithm with timeout                                   */
/* -------------------------------------------------------------------------- */
#define COMBO_FIFO_LEN       20
#define COMBO_TIMEOUT_MS     50
#define HOLD_TIME_THRESHOLD_MS 200  // 長押し判定の閾値

typedef struct {
    uint16_t keycode;
    uint16_t time_pressed;
    uint8_t  layer;       // 押下時のレイヤー
    bool     still_pressed; // 離されたかどうかのフラグ
} combo_event_t;

typedef struct {
    uint16_t a;
    uint16_t b;
    uint16_t out;
    uint8_t  layer;       // 対応レイヤー
} combo_pair_t;

// 長押し状態管理
typedef struct {
    uint16_t keycode;              // 長押し中のキーコード（0なら未設定）
    uint16_t time_confirmed;       // キーが確定した時刻
    bool     is_held;              // 長押し中フラグ
    uint16_t source_key_a;         // コンボを構成した物理キーA
    uint16_t source_key_b;         // コンボを構成した物理キーB
    bool     source_a_pressed;     // 物理キーAが押されているか
    bool     source_b_pressed;     // 物理キーBが押されているか
} hold_state_t;

// コンボ定義（順不同）
static const combo_pair_t combo_pairs[] PROGMEM = {
    // QWERTY layer combos
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

    // NUMBER layer combos
    {KC_1,    KC_7,    KC_4,     _NUMBER},
    {KC_2,    KC_8,    KC_5,     _NUMBER},
    {KC_3,    KC_9,    KC_6,     _NUMBER},
    {KC_DOT,  KC_MINS, KC_COMMA, _NUMBER},
    {KC_9,    KC_0,    KC_TAB,   _NUMBER},
    {KC_3,    KC_DZ,   KC_ESC,   _NUMBER},
    {KC_PGDN, KC_LEFT, KC_BSPC,  _NUMBER},
    {KC_PGUP, KC_HOME, KC_DEL,   _NUMBER},
};
#define COMBO_PAIR_COUNT (sizeof(combo_pairs) / sizeof(combo_pairs[0]))

static combo_event_t combo_fifo[COMBO_FIFO_LEN];
static uint8_t combo_fifo_len = 0;
static hold_state_t hold_state = {0, 0, false, 0, 0, false, false};  // 長押し状態の初期化

// combo_fifo内のエントリで、まだpressed扱いのものをreleaseに更新
static void mark_combo_released(uint16_t keycode) {
    for (uint8_t i = 0; i < combo_fifo_len; i++) {
        if (combo_fifo[i].keycode == keycode && combo_fifo[i].still_pressed) {
            combo_fifo[i].still_pressed = false;
            break;
        }
    }
}

static bool is_combo_candidate(uint16_t keycode) {
    // カスタムキーや特殊キーコードはコンボ対象外にする
    if (keycode >= SAFE_RANGE) {
        return false;
    }
    for (uint8_t i = 0; i < COMBO_PAIR_COUNT; i++) {
        combo_pair_t pair;
        memcpy_P(&pair, &combo_pairs[i], sizeof(pair));
        if (pair.a == keycode || pair.b == keycode) {
            return true;
        }
    }
    return false;
}

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

static void fifo_remove(uint8_t idx) {
    if (idx >= combo_fifo_len) return;
    for (uint8_t i = idx; i + 1 < combo_fifo_len; i++) {
        combo_fifo[i] = combo_fifo[i + 1];
    }
    combo_fifo_len--;
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
            // 前の長押しを終了
            if (hold_state.is_held) {
                unregister_code16(hold_state.keycode);
            }

            combo_pair_t pair;
            memcpy_P(&pair, hit, sizeof(pair));

            // 新しいキーを確定（物理キー情報も記録）
            hold_state.keycode = pair.out;
            hold_state.time_confirmed = timer_read();
            hold_state.is_held = true;  // 即座に押下状態にする
            hold_state.source_key_a = head_kc;
            hold_state.source_key_b = other_kc;
            hold_state.source_a_pressed = combo_fifo[0].still_pressed;
            hold_state.source_b_pressed = combo_fifo[i].still_pressed;

            register_code16(pair.out);
            fifo_remove(i); // 後ろから削除
            fifo_remove(0); // 先頭を削除
            return true;    // 1件処理したので呼び出し側で再試行
        }
    }
    return false;
}

// タイムアウト処理とコンボ解決を行う
static void combo_fifo_service(void) {
    while (combo_fifo_len > 0) {
        // 2: タイムアウトチェック（先頭）
        if (timer_elapsed(combo_fifo[0].time_pressed) > COMBO_TIMEOUT_MS) {
            // 直前まで押下されていたキーがあれば解除してから次を確定
            if (hold_state.is_held) {
                unregister_code16(hold_state.keycode);
            }
            // キーを確定する際に、長押し状態を更新（単独キー確定）
            hold_state.keycode = combo_fifo[0].keycode;
            hold_state.time_confirmed = timer_read();
            hold_state.is_held = true;  // 即座に押下状態にする
            hold_state.source_key_a = combo_fifo[0].keycode;
            hold_state.source_key_b = 0;  // コンボではないので1つだけ
            hold_state.source_a_pressed = combo_fifo[0].still_pressed;
            hold_state.source_b_pressed = false;
            register_code16(combo_fifo[0].keycode);
            fifo_remove(0);
            continue; // まだ処理できる要素があるかもしれないので続行
        }
        // 3: 先頭と他要素のペア探索
        if (combo_fifo_len >= 2 && resolve_combo_head()) {
            continue; // コンボが解決されたので再ループ
        }
        break; // 何もすることがない
    }
}

// ..................................................................... Keymaps
//
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
    // │  `  │  -  │  1  │  2  │  3  │ 00  │             │ PGU │ HOM │  ↑  │ END │ CAP │ JIS │
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
        KC_GRV, KC_MINS, KC_1, KC_2, KC_3,    KC_DZ,   KC_PGUP, KC_HOME, KC_UP,   KC_END,   KC_CAPS, TG_JIS,
        KC_ESC, KC_DOT,  KC_7, KC_8, KC_9,    KC_0,    KC_PGDN, KC_LEFT, KC_DOWN, KC_RIGHT, KC_LGUI, MO_FUN,
                                     MT_SPC,  KC_TRNS, MT_ENT,
                                     KC_LALT, KC_LCTL, KC_INT5, KC_INT4
    ),

    // FUNCTION
    // ┌─────┬─────┬─────┬─────┬─────┬─────┐             ┌─────┬─────┬─────┬─────┬─────┬─────┐
    // │ F1  │ F2  │ F3  │ F4  │ F5  │ F11 │             │ BRU │ VL0 │ VL- │ VL+ │ PSC │ SLP │
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
        KC_F1,  KC_F2, KC_F3, KC_F4, KC_F5,   KC_F11,  KC_BRIU, KC_MUTE, KC_VOLD, KC_VOLU, KC_PSCR, KC_SLEP,
        KC_ESC, KC_F6, KC_F7, KC_F8, KC_F9,   KC_F10,  KC_BRID, KC_MPRV, KC_MPLY, KC_MNXT, KC_LGUI, KC_TRNS,
                                     KC_TRNS, KC_TRNS, KC_TRNS,
                                     KC_TRNS, KC_TRNS, KC_F12,  KC_F13
    ),
};
// ..................................................................... Keymaps

// コンボを通常のQMKコンボ機能から除外したため、combo_t key_combos[] は削除しています。

bool process_record_user(uint16_t keycode, keyrecord_t *record) {

    // FIFO combo capture (press-only; releases are swallowed)
    if (is_combo_candidate(keycode)) {
        if (record->event.pressed) {
            if (combo_fifo_len < COMBO_FIFO_LEN) {
                combo_fifo[combo_fifo_len].keycode = keycode;
                combo_fifo[combo_fifo_len].layer   = get_highest_layer(layer_state | default_layer_state);
                combo_fifo[combo_fifo_len].time_pressed = timer_read();
                combo_fifo[combo_fifo_len].still_pressed = true;
                combo_fifo_len++;
            } else {
                // キューがいっぱいのときは失われないよう即時送信
                tap_code16(keycode);
            }
        } else {
            // pressedではなくreleaseの場合、FIFO内の該当キーを離された扱いにする
            mark_combo_released(keycode);
            // キーが離された時：物理キーの状態を更新
            if (hold_state.keycode != 0) {
                if (keycode == hold_state.source_key_a) {
                    hold_state.source_a_pressed = false;
                }
                if (keycode == hold_state.source_key_b) {
                    hold_state.source_b_pressed = false;
                }

                // すべての物理キーが離されたらキーを離す
                if (!hold_state.source_a_pressed && !hold_state.source_b_pressed) {
                    unregister_code16(hold_state.keycode);
                    hold_state.keycode = 0;
                    hold_state.is_held = false;
                }
            }
        }
        return false; // 通常処理を抑止
    }

    os_variant_t os = detected_host_os();
    bool is_mac = (os == OS_MACOS || os == OS_IOS);

    // Mod-Tap shift に対応したシフト+数字キーの処理
    if (IS_LAYER_ON(_NUMBER) && record->event.pressed && (get_mods() & MOD_MASK_SHIFT) && is_jis_mode) {
        uint16_t shifted_code = KC_NO;
        switch (keycode) {
            case KC_2: shifted_code = JP_AT; break;       // Shift+2 -> @
            case KC_6: shifted_code = JP_CIRC; break;     // Shift+6 -> ^
            case KC_7: shifted_code = JP_AMPR; break;     // Shift+7 -> &
            case KC_8: shifted_code = JP_ASTR; break;     // Shift+8 -> *
            case KC_9: shifted_code = JP_LPRN; break;     // Shift+9 -> (
            case KC_0: shifted_code = JP_RPRN; break;     // Shift+0 -> )
            case KC_MINS: shifted_code = JP_UNDS; break;  // Shift+- -> _
            case KC_SCLN: shifted_code = JP_COLN; break;  // Shift+; -> :
            default: break;
        }
        if (shifted_code != KC_NO) {
            // 修飾キーを一度削除して、記号キーを送信
            uint8_t mods = get_mods();
            del_mods(MOD_MASK_SHIFT);
            tap_code16(shifted_code);
            add_mods(mods);
            return false;
        }
    }

    switch (keycode) {
        case MT_TGL:  // MT_TGLキー
            if (record->tap.count > 0) {
                if (record->event.pressed) {
                    // _QWERTY と _GEMINI の間でトグル切り替えを行う
                    if (default_layer == 0) {
                        set_single_persistent_default_layer(_GEMINI);
                        tap_code16(is_mac ? KC_LNG1 : KC_INT4); // Macなら「かな」キー、Windowsなら「変換」キーを送信
                        default_layer = 1;
                    } else {
                        set_single_persistent_default_layer(_QWERTY);
                        tap_code16(is_mac ? KC_LNG2 : KC_INT5); // Macなら「英数」キー、Windowsなら「無変換」キーを送信
                        default_layer = 0;
                    }
                }
                return false;
            }
            return true;
        case TG_JIS:  // JISモード切替キー
            if (record->event.pressed) {
                is_jis_mode = !is_jis_mode;
                SEND_STRING(is_jis_mode ? "JIS ON" : "JIS OFF"); // JISモード切替キーを送信"")
            }
            return true; // JISモード切替キーは常に処理を続行
        case KC_DZ:
            if (record->event.pressed) {
                // 押された瞬間に0を2回送信
                SEND_STRING("00");
            }
            return true;
        case KC_LCTL:
            if (is_mac) {
                // Macの場合はCommandとして振る舞わせる
                if (record->event.pressed) {
                    register_code(KC_LGUI);
                } else {
                    unregister_code(KC_LGUI);
                }
                return false;
            }
            return true;
        case KC_LGUI:
            if (is_mac) {
                // Macの場合はControlとして振る舞わせる
                if (record->event.pressed) {
                    register_code(KC_LCTL);
                } else {
                    unregister_code(KC_LCTL);
                }
                return false;
            }
            return true;
        case KC_INT4:
            if (is_mac) {
                // Macの場合は「英数」キーとして振る舞わせる
                if (record->event.pressed) {
                    register_code(KC_LNG2);
                } else {
                    unregister_code(KC_LNG2);
                }
                return false;
            }
            return true;
        case KC_INT5:
            if (is_mac) {
                // Macの場合は「かな」キーとして振る舞わせる
                if (record->event.pressed) {
                    register_code(KC_LNG1);
                } else {
                    unregister_code(KC_LNG1);
                }
                return false;
            }
            return true;
        default:
            break;
    }
    // Mod-Tapキーのホールド時にLCTL・LGUIが割り当てられている場合の処理
    if ((keycode >= QK_MOD_TAP && keycode <= QK_MOD_TAP_MAX)) {
    // ホールド時の修飾キーが LCTL であるか確認
        if (QK_MOD_TAP_GET_MODS(keycode) == MOD_LCTL) {
            if (is_mac) {
                if (record->event.pressed) {
                    if (record->tap.count > 0) {
                        return true; // タップ時は通常のキーを送信
                    } else {
                        register_code(KC_LGUI); // Macなら Command を送信
                        return false;
                    }
                } else {
                    unregister_code(KC_LGUI);
                    return true; // 離した時のタップ処理は QMK 本体に任せる
                }
            }
        }
        // ホールド時の修飾キーが LGUI であるか確認
        else if (QK_MOD_TAP_GET_MODS(keycode) == MOD_LGUI) {
            if (is_mac) {
                if (record->event.pressed) {
                    if (record->tap.count > 0) {
                        return true; // タップ時は通常のキーを送信
                    } else {
                        register_code(KC_LCTL); // Macなら Control を送信
                        return false;
                    }
                } else {
                    unregister_code(KC_LCTL);
                    return true; // 離した時のタップ処理は QMK 本体に任せる
                }
            }
        }
    }
    return true;
}

// 毎スキャンでFIFOコンボの処理を行う
void matrix_scan_user(void) {
    combo_fifo_service();
    // 安全策: どの物理キーも押されていなければ強制的に解除
    if (hold_state.keycode != 0 && !hold_state.source_a_pressed && !hold_state.source_b_pressed) {
        unregister_code16(hold_state.keycode);
        hold_state.keycode = 0;
        hold_state.is_held = false;
    }
    // 長押し処理は不要（register_code16で既に押下状態）
}
