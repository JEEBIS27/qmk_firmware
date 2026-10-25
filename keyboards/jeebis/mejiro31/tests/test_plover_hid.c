// SPDX-License-Identifier: GPL-2.0-or-later
#include <assert.h>
#include <stdio.h>
#include "steno/plover.h"
#include "plover_hid.h"
#include "report.h"

static uint8_t data[8];
static unsigned reports;
void host_plover_hid_send(report_plover_hid_t *report) {
    assert(sizeof(*report) == 9 && report->report_id == 0x50);
    memcpy(data, report->data, sizeof(data));
    ++reports;
}
static void empty(void) {
    for (unsigned i = 0; i < 8; ++i) assert(data[i] == 0);
}
int main(void) {
    // Protocol positions, independently specified (not inferred from enums).
    const struct { uint16_t key; uint8_t bit; } cases[] = {
        {STN_S1,0}, {STN_TL,1}, {STN_KL,2}, {STN_PL,3}, {STN_WL,4}, {STN_HL,5}, {STN_RL,6},
        {STN_A,7}, {STN_O,8}, {STN_ST1,9}, {STN_E,10}, {STN_U,11}, {STN_FR,12}, {STN_RR,13},
        {STN_PR,14}, {STN_BR,15}, {STN_LR,16}, {STN_GR,17}, {STN_TR,18}, {STN_SR,19},
        {STN_DR,20}, {STN_ZR,21}, {STN_N1,22}, {STN_S2,23}, {STN_ST2,24}, {STN_ST3,25},
        {STN_ST4,26}, {STN_N2,27}, {STN_N3,28}, {STN_N4,29}
    };
    for (unsigned i = 0; i < sizeof(cases)/sizeof(*cases); ++i) {
        unsigned before = reports;
        mejiro_plover_key(cases[i].key, true);
        assert(reports == before + 1);
        for (unsigned byte = 0; byte < 8; ++byte)
            assert(data[byte] == (byte == cases[i].bit/8 ? 1U << (7-cases[i].bit%8) : 0));
        mejiro_plover_key(cases[i].key, false);
        assert(reports == before + 2);
        empty();
    }
    mejiro_plover_key(STN_TL, true);
    unsigned before = reports;
    mejiro_plover_key(STN_TL, true);
    mejiro_plover_key(STN_TL, false);
    assert(reports == before && data[0] == 0x40); // second physical owner remains
    mejiro_plover_key(STN_KL, true);
    assert(data[0] == 0x60);
    before = reports;
    mejiro_plover_reset();
    assert(reports == before + 1); // one complete all-up report
    empty();
    mejiro_plover_key(STN_TL, false); // suppressed old physical releases
    mejiro_plover_key(KC_A, true);
    assert(reports == before + 1);
    plover_hid_update(64, true); // out-of-range cannot corrupt the bitmap
    plover_hid_task();
    assert(reports == before + 1);
    puts("Plover HID: 30 key mappings, wire format, duplicate owners, reset, quick taps OK");
}
