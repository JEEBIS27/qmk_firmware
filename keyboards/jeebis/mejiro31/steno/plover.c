// SPDX-License-Identifier: GPL-2.0-or-later
#include "plover.h"
#include "plover_hid.h"
#include <string.h>

static uint8_t owners[PLV_BTN_COUNT];

static uint8_t button_for_key(uint16_t keycode) {
    switch (keycode) {
        case STN_S1: return PLV_BTN_S1;
        case STN_S2: return PLV_BTN_S2;
        case STN_TL: return PLV_BTN_TL;
        case STN_KL: return PLV_BTN_KL;
        case STN_PL: return PLV_BTN_PL;
        case STN_WL: return PLV_BTN_WL;
        case STN_HL: return PLV_BTN_HL;
        case STN_RL: return PLV_BTN_RL;
        case STN_A: return PLV_BTN_A;
        case STN_O: return PLV_BTN_O;
        case STN_ST1: return PLV_BTN_ST1;
        case STN_ST2: return PLV_BTN_ST2;
        case STN_ST3: return PLV_BTN_ST3;
        case STN_ST4: return PLV_BTN_ST4;
        case STN_E: return PLV_BTN_E;
        case STN_U: return PLV_BTN_U;
        case STN_FR: return PLV_BTN_FR;
        case STN_RR: return PLV_BTN_RR;
        case STN_PR: return PLV_BTN_PR;
        case STN_BR: return PLV_BTN_BR;
        case STN_LR: return PLV_BTN_LR;
        case STN_GR: return PLV_BTN_GR;
        case STN_TR: return PLV_BTN_TR;
        case STN_SR: return PLV_BTN_SR;
        case STN_DR: return PLV_BTN_DR;
        case STN_ZR: return PLV_BTN_ZR;
        case STN_N1: return PLV_BTN_N1;
        case STN_N2: return PLV_BTN_N2;
        case STN_N3: return PLV_BTN_N3;
        case STN_N4: return PLV_BTN_N4;
        default: return PLV_BTN_COUNT;
    }
}

void mejiro_plover_key(uint16_t keycode, bool pressed) {
    uint8_t button = button_for_key(keycode);
    if (button == PLV_BTN_COUNT) return;
    if (pressed) {
        if (owners[button]++ == 0) plover_hid_update(button, true);
    } else if (owners[button] && --owners[button] == 0) {
        plover_hid_update(button, false);
    }
    // Preserve each transition, including a tap or reset in the same scan.
    plover_hid_task();
}

void mejiro_plover_reset(void) {
    for (uint8_t button = 0; button < PLV_BTN_COUNT; ++button)
        if (owners[button]) plover_hid_update(button, false);
    memset(owners, 0, sizeof(owners));
    plover_hid_task();
}
