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

#include QMK_KEYBOARD_H
#include "keymap_japanese.h"

// グローバル変数の宣言（各キーマップファイルで定義）
extern bool is_jis_mode;

/*---------------------------------------------------------------------------------------------------*/
/*--------------------------------------------JIS×US変換---------------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/

/**
 * JIS×US配列を変換する関数
 * @param kc キーコード
 * @param shifted Shiftキーが押されているかどうか
 * @return 変換後のキーコード
 */
static inline uint16_t jis_transform(uint16_t kc, bool shifted) {
    if (!is_jis_mode) return kc;
    switch (kc) {
        case KC_1: return shifted ? JP_EXLM : KC_1;   // ! / 1
        case KC_2: return shifted ? JP_AT   : KC_2;   // @ / 2
        case KC_3: return shifted ? JP_HASH : KC_3;   // # / 3
        case KC_4: return shifted ? JP_DLR  : KC_4;   // $ / 4
        case KC_5: return shifted ? JP_PERC : KC_5;   // % / 5
        case KC_6: return shifted ? JP_CIRC : KC_6;   // ^ / 6
        case KC_7: return shifted ? JP_AMPR : KC_7;   // & / 7
        case KC_8: return shifted ? JP_ASTR : KC_8;   // * / 8
        case KC_9: return shifted ? JP_LPRN : KC_9;   // ( / 9
        case KC_0: return shifted ? JP_RPRN : KC_0;   // ) / 0

        case KC_GRV:  return shifted ? JP_TILD : JP_GRV;   // ~ / `
        case KC_MINS: return shifted ? JP_UNDS : JP_MINS;  // _ / -
        case KC_EQL:  return shifted ? JP_PLUS : JP_EQL;   // + / =
        case KC_LBRC: return shifted ? JP_LCBR : JP_LBRC;  // { / [
        case KC_RBRC: return shifted ? JP_RCBR : JP_RBRC;  // } / ]
        case KC_BSLS: return shifted ? JP_PIPE : JP_BSLS;  // | / ￥
        case KC_SCLN: return shifted ? JP_COLN : JP_SCLN;  // : / ;
        case KC_QUOT: return shifted ? JP_DQUO : JP_QUOT;  // " / '
        case KC_COMM: return shifted ? JP_LABK : JP_COMM;  // < / ,
        case KC_DOT:  return shifted ? JP_RABK : JP_DOT;   // > / .
        case KC_SLSH: return shifted ? JP_QUES : JP_SLSH;  // ? / /
        default: return kc;
    }
}

/**
 * JISモード時にShiftを一時的に無効化してキーを入力する
 * @param kc キーコード
 */
static inline void tap_code16_unshifted(uint16_t kc) {
    uint8_t saved_mods      = get_mods();
    uint8_t saved_weak_mods = get_weak_mods();
    uint8_t saved_osm       = get_oneshot_mods();

    del_mods(MOD_MASK_SHIFT);
    del_weak_mods(MOD_MASK_SHIFT);
    clear_oneshot_mods();
    send_keyboard_report();

    tap_code16(kc);

    set_mods(saved_mods);
    set_weak_mods(saved_weak_mods);
    set_oneshot_mods(saved_osm);
    send_keyboard_report();
}

/**
 * JIS変換対象でシフト解除が必要なキーか判定
 * @param kc キーコード
 * @param shifted Shiftキーが押されているかどうか
 * @return シフト解除が必要な場合はtrue
 */
static inline bool is_jis_shift_target(uint16_t kc, bool shifted) {
    if (!is_jis_mode || !shifted) return false;
    switch (kc) {
        case KC_1: case KC_2: case KC_3: case KC_4: case KC_5:
        case KC_6: case KC_7: case KC_8: case KC_9: case KC_0:
        case KC_GRV: case KC_MINS: case KC_EQL:
        case KC_LBRC: case KC_RBRC: case KC_BSLS:
        case KC_SCLN: case KC_QUOT: case KC_COMM: case KC_DOT: case KC_SLSH:
            return true;
        default:
            return false;
    }
}
