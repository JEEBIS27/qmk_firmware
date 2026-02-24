#!/usr/bin/env python3
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
KEYMAPS_DIR = ROOT / "keyboards" / "jeebis" / "mejiro31" / "keymaps"
BASE_NAME = "en_graphite"

ALT_MAP_RE = re.compile(
    r"static const alt_mapping_t alt_mappings\[\] PROGMEM = \{\n.*?\n\};",
    re.DOTALL,
)

def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def write_text(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8")


def main() -> int:
    base_path = KEYMAPS_DIR / BASE_NAME / "keymap.c"
    base_text = read_text(base_path)
    base_alt_match = ALT_MAP_RE.search(base_text)
    if not base_alt_match:
        raise SystemExit("alt_mappings block not found in en_graphite")

    for keymap_dir in sorted(KEYMAPS_DIR.iterdir()):
        if not keymap_dir.is_dir():
            continue
        name = keymap_dir.name
        if not name.startswith("en_") or name == BASE_NAME:
            continue

        target_path = keymap_dir / "keymap.c"
        if not target_path.exists():
            continue

        target_text = read_text(target_path)

        target_alt_match = ALT_MAP_RE.search(target_text)
        if not target_alt_match:
            print(f"skip {name}: alt_mappings not found")
            continue

        new_text = base_text
        new_text = ALT_MAP_RE.sub(target_alt_match.group(0), new_text, count=1)

        write_text(target_path, new_text)
        print(f"updated {name}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
