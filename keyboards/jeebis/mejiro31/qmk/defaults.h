// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
// Default differences are ordinary binding data, never processing branches.
#if MJI_EN_PROFILE
#    define MJI_TEXT_LEFT KC_EXLM
#    define MJI_TEXT_RIGHT KC_QUES
#    define MJI_TEXT_SHIFT_LEFT KC_AMPR
#    define MJI_TEXT_SHIFT_RIGHT KC_PIPE
#    define MJI_TEXT_SHIFT_COMBO KC_HASH
#    define MJI_NUM_LEFT KC_EXLM
#    define MJI_NUM_RIGHT KC_QUES
#    define MJI_NUM_SHIFT_LEFT KC_AMPR
#    define MJI_NUM_SHIFT_RIGHT KC_PIPE
#    define MJI_NUM_SHIFT_HOLD_LEFT KC_NO
#    define MJI_NUM_SHIFT_HOLD_RIGHT KC_NO
#else
#    define MJI_TEXT_LEFT KC_LNG2
#    define MJI_TEXT_RIGHT KC_LNG1
#    define MJI_TEXT_SHIFT_LEFT KC_EXLM
#    define MJI_TEXT_SHIFT_RIGHT KC_QUES
#    define MJI_TEXT_SHIFT_COMBO KC_AT
#    define MJI_NUM_LEFT KC_AMPR
#    define MJI_NUM_RIGHT KC_PIPE
#    define MJI_NUM_SHIFT_LEFT KC_NO
#    define MJI_NUM_SHIFT_RIGHT KC_NO
#    define MJI_NUM_SHIFT_HOLD_LEFT KC_LCTL
#    define MJI_NUM_SHIFT_HOLD_RIGHT KC_LALT
#endif
// Matrix cells without switches are the editable virtual combo positions.
// Hold layer N + MJI_HOLD_OFFSET belongs to Tap layer N.
const uint16_t PROGMEM
    keymaps[][MATRIX_ROWS][MATRIX_COLS] =
        {
            [0] =
                {
                    {KC_GRV, KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_MINS},
                    {KC_NO, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_BSLS},
                    {KC_A, KC_S, KC_D, KC_F, KC_SPC, MJI_MODE, KC_ENT, KC_G, KC_H, KC_J, KC_K, KC_L},
                    {KC_SCLN, KC_QUOT, KC_ESC, KC_TAB, KC_NO, KC_NO, MJI_TEXT_LEFT, MJI_TEXT_RIGHT, KC_DEL, KC_BSPC, KC_AT, KC_NO},
                },
            [1] =
                {
                    {KC_TILD, KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_UNDS},
                    {KC_NO, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_LABK, KC_RABK, KC_QUES, KC_PIPE},
                    {KC_A, KC_S, KC_D, KC_F, KC_SPC, MJI_MODE, KC_ENT, KC_G, KC_H, KC_J, KC_K, KC_L},
                    {KC_SCLN, KC_QUOT, KC_ESC, KC_TAB, KC_NO, KC_NO, MJI_TEXT_SHIFT_LEFT, MJI_TEXT_SHIFT_RIGHT, KC_DEL, KC_BSPC, MJI_TEXT_SHIFT_COMBO, KC_NO},
                },
            [2] =
                {
                    {MJI_MEJIRO, MJI_00, KC_1, KC_2, KC_3, KC_MINS, KC_PGUP, KC_HOME, KC_UP, KC_END, KC_CAPS, MJI_ALT},
                    {KC_NO, KC_0, KC_7, KC_8, KC_9, KC_DOT, KC_PGDN, KC_LEFT, KC_DOWN, KC_RIGHT, KC_NO, KC_NO},
                    {MJI_000, KC_4, KC_5, KC_6, KC_SPC, KC_TRNS, KC_ENT, KC_COMM, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_NO, KC_NO, KC_ESC, KC_TAB, KC_NO, KC_NO, MJI_NUM_LEFT, MJI_NUM_RIGHT, KC_DEL, KC_BSPC, KC_HASH, KC_NO},
                },
            [3] =
                {
                    {MJI_MEJIRO, KC_PERC, KC_LCBR, KC_LBRC, KC_LPRN, KC_LABK, KC_PGUP, KC_HOME, KC_UP, KC_END, KC_CAPS, MJI_ALT},
                    {KC_NO, KC_DLR, KC_RCBR, KC_RBRC, KC_RPRN, KC_RABK, KC_PGDN, KC_LEFT, KC_DOWN, KC_RIGHT, KC_NO, KC_NO},
                    {KC_SLSH, KC_ASTR, KC_EQL, KC_PLUS, KC_SPC, KC_TRNS, KC_ENT, KC_CIRC, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_NO, KC_NO, KC_ESC, KC_TAB, KC_NO, KC_NO, MJI_NUM_SHIFT_LEFT, MJI_NUM_SHIFT_RIGHT, KC_DEL, KC_BSPC, KC_HASH, KC_NO},
                },
            [4] =
                {
                    {MJI_JIS, KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_BRIU, KC_MUTE, KC_VOLD, KC_VOLU, KC_PSCR, KC_TRNS},
                    {KC_NO, KC_F6, KC_F7, KC_F8, KC_F9, KC_F10, KC_BRID, KC_MPRV, KC_MPLY, KC_MNXT, KC_NO, KC_TRNS},
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_SPC, KC_TRNS, KC_ENT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_TRNS, KC_TRNS, KC_F11, KC_F12, KC_NO, KC_NO, KC_NO, KC_NO},
                },
            [5] =
                {
                    {STN_N1, STN_S1, STN_TL, STN_PL, STN_HL, STN_ST1, STN_ST3, STN_FR, STN_PR, STN_LR, STN_TR, STN_DR},
                    {STN_N2, STN_S2, STN_KL, STN_WL, STN_RL, STN_ST2, STN_ST4, STN_RR, STN_BR, STN_GR, STN_SR, STN_ZR},
                    {KC_NO, KC_NO, KC_NO, KC_NO, STN_N3, MJI_MODE, STN_N4, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_NO, KC_NO, KC_NO, KC_NO, STN_A, STN_O, STN_E, STN_U, KC_NO, KC_NO, KC_NO, KC_NO},
                },
            [6] =
                {
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_LGUI, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_LSFT, MO(MJI_NUMBER), KC_RSFT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_LALT, KC_LCTL, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                },
            [7] =
                {
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_LGUI, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_LSFT, MO(MJI_NUMBER), KC_RSFT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_LALT, KC_LCTL, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                },
            [8] =
                {
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_LGUI, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_LGUI, MO(MJI_FUNCTION)},
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_LSFT, KC_TRNS, KC_RSFT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_LALT, KC_LCTL, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                },
            [9] =
                {
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_LGUI, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_LGUI, MO(MJI_FUNCTION)},
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_LSFT, KC_TRNS, KC_RSFT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_LALT, KC_LCTL, MJI_NUM_SHIFT_HOLD_LEFT, MJI_NUM_SHIFT_HOLD_RIGHT, KC_NO, KC_NO, KC_NO, KC_NO},
                },
            [10] =
                {
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_TRNS},
                    {KC_LGUI, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_LGUI, KC_TRNS},
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_LSFT, KC_TRNS, KC_RSFT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_LALT, KC_LCTL, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                },
            [12] = {{KC_NO}},
            [13] = {{KC_NO}},
            [11] =
                {
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, MO(MJI_NUMBER), KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                    {KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO},
                },
};
