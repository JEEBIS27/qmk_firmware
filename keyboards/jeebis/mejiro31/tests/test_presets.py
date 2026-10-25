#!/usr/bin/env python3
"""Check that existing preset recipes select portable engine maps."""
from pathlib import Path
import re
import runpy

root = Path(__file__).resolve().parents[1]
catalog = (root / "layouts/catalog.h").read_text()
layouts = set(re.findall(r"static const mji_layout_entry_t mji_layout_(\w+)\[", catalog))
count = 0
for name in ("jp_default", "en_default"):
    config = runpy.run_path(str(root / "keymaps" / name / "build_firmware_presets.py"))["CONFIG"]
    from firmware_preset_builder import load_presets, render_keymap, VALID_LAYOUTS
    assert layouts == VALID_LAYOUTS
    source = config.keymap_path.read_text()
    for preset in load_presets(config):
        rendered = render_keymap(source, preset, config.rewrite_rules)
        choices = dict(re.findall(r"^#define MJI_(ENGLISH|JAPANESE)_LAYOUT mji_layout_(\w+)$", rendered, re.M))
        assert set(choices) == {"ENGLISH", "JAPANESE"}
        assert set(choices.values()) <= layouts
        expected_en = preset.get("english_layout", preset.get("layout"))
        expected_jp = preset.get("japanese_layout", preset.get("layout"))
        assert choices == {"ENGLISH": expected_en, "JAPANESE": expected_jp}
        count += 1
print(f"preset engine-map selections: {count} recipes OK")
