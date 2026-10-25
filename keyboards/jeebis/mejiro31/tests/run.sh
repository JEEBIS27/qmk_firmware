#!/bin/sh
set -eu
cd "$(dirname "$0")"
build=$(mktemp -d /tmp/mejiro-input-test.XXXXXX)
trap 'rm -rf "$build"' EXIT HUP INT TERM
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic -g ${CFLAGS:-} ../mejiro_input.c ../mejiro_layout.c test_input.c -o "$build/test_input"
"$build/test_input"
qmk_root=../../../..
for profile in jp_default en_default; do
    profile_flags=
    if [ "$profile" = en_default ]; then profile_flags=-DMJI_TEST_EN; fi
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -g ${CFLAGS:-} \
        -DQMK_KEYBOARD_H='"qmk_stub.h"' -I qmk_stubs -I .. \
        -I "$qmk_root/quantum" -I "$qmk_root/quantum/sequencer" -I "$qmk_root/quantum/keymap_extras" \
        ../mejiro_input.c ../mejiro_layout.c ${profile_flags} test_qmk_adapter.c -o "$build/test_qmk_adapter"
    "$build/test_qmk_adapter"
done
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic -g ${CFLAGS:-} ../mejiro_layout.c test_layout.c -o "$build/test_layout"
"$build/test_layout"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic -g ${CFLAGS:-} test_catalog.c -o "$build/test_catalog"
"$build/test_catalog"
host_target=$(rustc -vV | sed -n 's/^host: //p')
CARGO_TARGET_DIR="$build/rust" cargo build --locked --quiet --release --target "$host_target" \
    --manifest-path ../steno/rust/Cargo.toml
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -g ${CFLAGS:-} \
    -DQMK_KEYBOARD_H='"qmk_stub.h"' -I qmk_stubs -I .. \
    -I "$qmk_root/quantum" -I "$qmk_root/quantum/sequencer" -I "$qmk_root/quantum/keymap_extras" \
    ../steno/chord.c ../steno/engine.c test_steno_engine.c \
    "$build/rust/$host_target/release/libmejiro_qmk.a" -o "$build/test_steno_engine"
"$build/test_steno_engine"
for profile in jp_default en_default; do
    profile_flags=
    if [ "$profile" = en_default ]; then profile_flags=-DMJI_TEST_EN; fi
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -g ${CFLAGS:-} \
        -DQMK_KEYBOARD_H='"qmk_stub.h"' -I qmk_stubs -I .. \
        -I "$qmk_root/quantum" -I "$qmk_root/quantum/sequencer" -I "$qmk_root/quantum/keymap_extras" \
        ../mejiro_input.c ../mejiro_layout.c ../steno/engine.c ${profile_flags} test_qmk_recording.c \
        "$build/rust/$host_target/release/libmejiro_qmk.a" -o "$build/test_qmk_recording"
    "$build/test_qmk_recording"
done
PYTHONDONTWRITEBYTECODE=1 python3 test_presets.py
