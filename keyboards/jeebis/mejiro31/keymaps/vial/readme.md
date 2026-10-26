# Mejiro31 Vial / browser configurator

All Mejiro31 keymaps now use Vial protocol 6, UID `M31WEB01`, dynamic
12-layer Tap/Hold maps and the browser settings extension. Existing declarative
language/preset definitions and the unified input engine are preserved.

Use the same commands as before, from this repository:

```sh
make jeebis/mejiro31:jp_default
qmk compile -kb jeebis/mejiro31 -km en_default
```

Both entry points transparently prepare a pinned Vial core under
`.build/mejiro31-vial/` and build **this checkout's keyboard sources** with their
original keymap names. The ordinary `jp_default` retains Graphite for English
and QWERTY for Japanese; `en_default` retains QWERTY for both. Preset-builder
edits are copied into the prepared tree, so their existing commands also produce
site-compatible UF2s. All other keyboards keep their normal QMK build path.

Vial requires its quantum core, so the two build entry points route Mejiro31
through `build_vial.py`; there is no separate non-Vial Mejiro31 build choice.
The helper fetches pinned Vial-QMK and Mejiro_Engine commits, preserves Plover
HID through `plover-hid.patch`, and fixes unlock cancellation. It does not
replace this repository's core or rewrite this checkout's keyboard definitions.
Use `MJI_VIAL_WORK_DIR` to override the cache directory.
Local edits are copied; use a clean checkout for reproducible release builds.

For site-distributed variants with explicitly selected initial layouts, the
internal helper also supports:

```sh
python3 keyboards/jeebis/mejiro31/build_vial.py --profile jp --layout qwerty
python3 keyboards/jeebis/mejiro31/build_vial.py --profile en --layout graphite
```

Prerequisites: Git, Python 3, Make, QMK CLI and its dependencies (install the
prepared Vial tree's `requirements.txt` if necessary), Arm GNU Toolchain,
Rust >=1.88 and the `thumbv6m-none-eabi` Rust target. Ensure the compiler and QMK
CLI are on PATH. Prepare without compiling with `--prepare-only`; `--skip-submodules`
is intended for host-only testing or an already initialized cache.

Named builds copy `jeebis_mejiro31_<keymap>.uf2` to this repository root, as before.
All verified UF2 files and their SHA-256 checksums are also written to the cache:

```
.build/mejiro31-vial/artifacts/mejiro31_vial_jp_qwerty.uf2
.build/mejiro31-vial/artifacts/mejiro31_vial_en_graphite.uf2
```

`--profile all` builds both languages; `--layout` supports QWERTY and Graphite
as initial defaults. Other alternate layouts can be selected from the site after
connection. All 18 runtime layout tables remain compiled into the firmware.
`web_layouts.h` defines persistent numeric IDs: append new entries, never reorder.

Write the UF2 to the RP2040 BOOTSEL drive, then reconnect normally. The browser
editor must expect protocol 6, UID `M31WEB01`, 12 layers and extension signature
`M31WEB01`. It uses the Raw HID interface at usage page `0xff60`, usage `0x61`.
The site source and protocol reference are maintained in
[JEEBIS27/Mejiro31](https://github.com/JEEBIS27/Mejiro31).

Unlocking requires holding the top-left and top-right physical keys. While
unlocking, the unified input engine is paused. Do not enable Vial's dynamic
Combo/Tap Dance/QMK Settings: this keyboard's engine owns combo and Tap/Hold
semantics. The additional 16 Vial positions are outputs of fixed combos, not
extra physical keys. Layers 0..5 are Tap and 6..11 are their Hold counterparts.
The Vial firmware uses a separate user-settings version; back up settings before
switching firmware variants. This keymap does not add browser flashing.

## Verification

After preparation, run the real adapter and protocol handler with host stubs:

```sh
python3 keyboards/jeebis/mejiro31/tests/run_vial_tests.py \
  --source-dir .build/mejiro31-vial/vial
```

This covers both language defaults, configuration persistence, lock checks,
invalid settings, runtime layout changes, held-key cleanup and unlock barriers.
The build helper verifies RP2040 UF2 blocks, address bounds, the extension
signature and SHA-256. These checks do **not** verify physical USB enumeration,
WebHID access, flashing, or actual IME/Plover behavior. Those remain required
before treating this as a hardware-validated release.
