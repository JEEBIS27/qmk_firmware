// SPDX-License-Identifier: GPL-2.0-or-later
#include "../layouts/catalog.h"
#include <assert.h>
#include <stdio.h>
static unsigned total;
static void     check(const mji_layout_entry_t *entries, unsigned count) {
    assert(count == 33);
    for (unsigned i = 0; i < count; i++) {
        assert(entries[i].base < 256 && entries[i].plain.code < 256 && entries[i].shifted.code < 256);
        for (unsigned j = 0; j < i; j++)
            assert(entries[i].base != entries[j].base);
    }
    total += count;
}
int main(void) {
    check(mji_layout_qwerty, sizeof(mji_layout_qwerty) / sizeof(mji_layout_qwerty[0]));
    check(mji_layout_graphite, sizeof(mji_layout_graphite) / sizeof(mji_layout_graphite[0]));
    check(mji_layout_colemak, sizeof(mji_layout_colemak) / sizeof(mji_layout_colemak[0]));
    check(mji_layout_colemak_dh, sizeof(mji_layout_colemak_dh) / sizeof(mji_layout_colemak_dh[0]));
    check(mji_layout_dvorak, sizeof(mji_layout_dvorak) / sizeof(mji_layout_dvorak[0]));
    check(mji_layout_workman, sizeof(mji_layout_workman) / sizeof(mji_layout_workman[0]));
    check(mji_layout_handsdown_neu, sizeof(mji_layout_handsdown_neu) / sizeof(mji_layout_handsdown_neu[0]));
    check(mji_layout_sturdy, sizeof(mji_layout_sturdy) / sizeof(mji_layout_sturdy[0]));
    check(mji_layout_engram, sizeof(mji_layout_engram) / sizeof(mji_layout_engram[0]));
    check(mji_layout_gallium, sizeof(mji_layout_gallium) / sizeof(mji_layout_gallium[0]));
    check(mji_layout_canary, sizeof(mji_layout_canary) / sizeof(mji_layout_canary[0]));
    check(mji_layout_astarte, sizeof(mji_layout_astarte) / sizeof(mji_layout_astarte[0]));
    check(mji_layout_boo, sizeof(mji_layout_boo) / sizeof(mji_layout_boo[0]));
    check(mji_layout_eucalyn, sizeof(mji_layout_eucalyn) / sizeof(mji_layout_eucalyn[0]));
    check(mji_layout_eucalyn_biacco, sizeof(mji_layout_eucalyn_biacco) / sizeof(mji_layout_eucalyn_biacco[0]));
    check(mji_layout_merlin, sizeof(mji_layout_merlin) / sizeof(mji_layout_merlin[0]));
    check(mji_layout_o24, sizeof(mji_layout_o24) / sizeof(mji_layout_o24[0]));
    check(mji_layout_tomisuke, sizeof(mji_layout_tomisuke) / sizeof(mji_layout_tomisuke[0]));
    assert(total == 594);
    puts("portable catalog: 18 layouts / 594 entries OK");
}
