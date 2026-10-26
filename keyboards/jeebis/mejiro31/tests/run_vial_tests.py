#!/usr/bin/env python3
"""Host-test the actual prepared adapter and dedicated WebHID settings handler."""
import argparse
from pathlib import Path
import subprocess
import tempfile
KB = Path(__file__).resolve().parents[1]
ROOT = KB.parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--source-dir', type=Path, default=ROOT)
args = parser.parse_args()
source = args.source_dir.resolve()
kb = source / 'keyboards/jeebis/mejiro31'
with tempfile.TemporaryDirectory(prefix='mejiro-web-tests-') as work:
    work = Path(work)
    (work / 'dynamic_keymap.h').write_text('#define dynamic_keymap_get_keycode keycode_at_keymap_location\n')
    (work / 'vial.h').write_text('extern int vial_unlocked;\nextern int vial_unlock_in_progress;\n')
    (work / 'via.h').write_text('bool via_eeprom_is_valid(void);\n')
    fixture = (kb / 'tests/test_qmk_adapter.c').read_text().replace('int main(void)', 'int adapter_regression_main(void)')
    for profile in ['jp', 'en']:
        fixture = fixture.replace(f'"../keymaps/{profile}_default/keymap.c"', f'"{kb}/keymaps/vial/keymap.c"')
    (work / 'adapter_fixture.c').write_text(fixture)
    flags = ['-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', '-g', '-DDYNAMIC_KEYMAP_LAYER_COUNT=12', '-DMJI_WEB_EN_DEFAULT=0', '-DMJI_WEB_JP_DEFAULT=0', '-DQMK_KEYBOARD_H="qmk_stub.h"']
    for include in [work, kb / 'tests/qmk_stubs', kb, kb / 'keymaps/vial', source / 'quantum', source / 'platforms', source / 'quantum/sequencer', source / 'quantum/keymap_extras']:
        flags += ['-I', str(include)]
    for profile in ['jp', 'en']:
        command = ['cc', *flags, *(['-DMJI_TEST_EN', '-DMJI_EN_PROFILE=1'] if profile == 'en' else []), str(kb / 'mejiro_input.c'), str(kb / 'mejiro_layout.c'), str(kb / 'steno/plover.c'), str(source / 'quantum/plover_hid.c'), str(KB / 'tests/test_vial_protocol.c'), '-o', str(work / 'adapter')]
        subprocess.run(command, check=True)
        subprocess.run([str(work / 'adapter')], check=True)
