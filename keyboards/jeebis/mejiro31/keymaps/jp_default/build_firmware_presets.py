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

ALT_EN_PATTERN = re.compile(
    r"^#define MJI_ENGLISH_LAYOUT mji_layout_[a-z0-9_]+.*$",
    re.MULTILINE,
)
ALT_JP_PATTERN = re.compile(
    r"^#define MJI_JAPANESE_LAYOUT mji_layout_[a-z0-9_]+.*$",
    re.MULTILINE,
)

CONFIG = PresetBuilderConfig(
    script_dir=SCRIPT_DIR,
    keyboard="jeebis/mejiro31",
    keymap="jp_default",
    build_stem="mejiro31_jp",
    preset_fields=("firmware_name", "english_layout", "japanese_layout"),
    layout_fields=("english_layout", "japanese_layout"),
    description=(
        "Build firmware presets for jeebis/mejiro31:jp_default by temporarily "
        "selecting English / Japanese engine maps in keymap.c."
    ),
    list_format=lambda preset: (
        f"{preset['firmware_name']}: "
        f"en={preset['english_layout']}, jp={preset['japanese_layout']}"
    ),
    rewrite_rules=(
        RewriteRule(
            pattern=ALT_EN_PATTERN,
            replacement_factory=lambda preset: (
                "#define MJI_ENGLISH_LAYOUT "
                f"mji_layout_{preset['english_layout']}"
            ),
            error_message="Could not select English layout in keymap.c.",
        ),
        RewriteRule(
            pattern=ALT_JP_PATTERN,
            replacement_factory=lambda preset: (
                "#define MJI_JAPANESE_LAYOUT "
                f"mji_layout_{preset['japanese_layout']}"
            ),
            error_message="Could not select Japanese layout in keymap.c.",
        ),
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
