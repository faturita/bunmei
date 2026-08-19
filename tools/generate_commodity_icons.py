#!/usr/bin/env python3
"""Generate 7x7 city commodity icons from the special-resource terrain images.

Reads the SPECIALRESOURCE -> image path mapping (initTiles) and the
SPECIALRESOURCE -> COMMODITIES mapping (initCommodities) straight out of
src/tiles.cpp, so the icon set can't drift from the game's own data. For
each commodity, picks the first special resource that maps to it
(declaration order in tiles.cpp, e.g. furs <- DOE) and downsizes its
terrain image (15x15) to the 7x7 city-icon format used by
assets/assets/city/*.png (e.g. culture.png, food.png), via `sips`.

Usage: python3 tools/generate_commodity_icons.py
Output: assets/assets/city/<commodity>.png, one per COMMODITIES enum value.
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TILES_CPP = ROOT / "src" / "tiles.cpp"
OUT_DIR = ROOT / "assets" / "assets" / "city"
ICON_SIZE = 7


def parse_tiles_cpp(text):
    resource_image = dict(re.findall(r'tiles\[(\w+)\]\s*=\s*"([^"]+)"', text))
    commodity_of = re.findall(r'commodityxresource\[(\w+)\]\s*=\s*(\w+);', text)
    return resource_image, commodity_of


def main():
    text = TILES_CPP.read_text()
    resource_image, commodity_of = parse_tiles_cpp(text)

    if not commodity_of:
        sys.exit("No commodityxresource[...] mappings found in tiles.cpp")

    # First resource declared for a commodity wins (deterministic pick
    # among multiple resources sharing a commodity, e.g. DOE/GAME/SEAL -> furs).
    commodity_source = {}
    for resource, commodity in commodity_of:
        commodity_source.setdefault(commodity, resource)

    OUT_DIR.mkdir(parents=True, exist_ok=True)

    for commodity, resource in commodity_source.items():
        src = resource_image.get(resource)
        if src is None:
            print(f"skip {commodity}: no tiles[{resource}] image", file=sys.stderr)
            continue
        src_path = ROOT / src
        dst_path = OUT_DIR / f"{commodity}.png"
        subprocess.run(
            ["sips", "-z", str(ICON_SIZE), str(ICON_SIZE), str(src_path), "--out", str(dst_path)],
            check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        )
        print(f"{commodity}.png  <-  {resource} ({src})")


if __name__ == "__main__":
    main()
