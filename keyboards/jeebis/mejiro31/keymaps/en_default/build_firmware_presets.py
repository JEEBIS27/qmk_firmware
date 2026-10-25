#!/usr/bin/env python3

from __future__ import annotations

import sys
from pathlib import Path
import re

SCRIPT_DIR = Path(__file__).resolve().parent
SHARED_DIR = SCRIPT_DIR.parent
if str(SHARED_DIR) not in sys.path:
    sys.path.insert(0, str(SHARED_DIR))

from firmware_preset_builder import PresetBuilderConfig, RewriteRule, run_builder

LAYOUT_PATTERNS = tuple(re.compile(rf"^#define MJI_{language}_LAYOUT mji_layout_[a-z0-9_]+.*$", re.MULTILINE) for language in ("ENGLISH", "JAPANESE"))

CONFIG = PresetBuilderConfig(
    script_dir=SCRIPT_DIR,
    keyboard="jeebis/mejiro31",
    keymap="en_default",
    build_stem="jeebis_mejiro31_en_default",
    preset_fields=("firmware_name", "layout"),
    layout_fields=("layout",),
    description=(
        "Build firmware presets for jeebis/mejiro31:en_default by temporarily "
        "selecting the engine layout in keymap.c."
    ),
    list_format=lambda preset: f"{preset['firmware_name']}: layout={preset['layout']}",
    rewrite_rules=tuple(
        RewriteRule(
            pattern=pattern,
            replacement_factory=lambda preset, language=language: f"#define MJI_{language}_LAYOUT mji_layout_{preset['layout']}",
            error_message=f"Could not select {language} layout in keymap.c.",
        )
        for language, pattern in zip(("ENGLISH", "JAPANESE"), LAYOUT_PATTERNS)
    ),
    output_subdir=".build/export",
)


if __name__ == "__main__":
    try:
        raise SystemExit(run_builder(CONFIG))
    except KeyboardInterrupt:
        print("Build cancelled.", file=sys.stderr)
        raise SystemExit(130)
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        raise SystemExit(1)
