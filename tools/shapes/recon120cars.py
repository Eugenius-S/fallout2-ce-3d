"""Task 120 slice 2 recon: which scenery arts are the cars/trucks/baths, and
do they hold solids today in the denbus1 cache scene?

    py -3 tools/shapes/recon120cars.py

Prints every scenery FRM whose name matches car/truck/bath (index, direction
count, side map), then decodes the denbus1 cache scene's prop rows for those
indices: board/round (box/drum solid flags), seen, fit params.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "scene_export"))
sys.path.insert(0, str(ROOT / "tools" / "scene_export" / "vendor"))

from export import ReadOnlyGameFiles  # noqa: E402
from f2lib.frm import Frm  # noqa: E402

GAME = ROOT / "game"
SCENE = ROOT / "build" / "cache-122" / "denbus1" / "0" / "scene.json"
MATCH = ("car", "truck", "bath")


def main():
    gf = ReadOnlyGameFiles(GAME)
    hits = {}
    try:
        for index, name in enumerate(gf.art_list(2)):
            if not name:
                continue
            low = name.lower()
            if not low.endswith(".frm") or not any(m in low for m in MATCH):
                continue
            path = f"art/scenery/{name}"
            if not gf.exists(path):
                continue
            frm = Frm.from_bytes(gf.read(path))
            hits[index] = (low, frm.direction_count, list(frm.direction_map))
            print(f"scenery[{index:4d}] {low:24s} dirs={frm.direction_count} sides={list(frm.direction_map)}")
    finally:
        gf.close()

    scene = json.loads(SCENE.read_text())
    print(f"\n{SCENE.name}: top keys {sorted(scene.keys())}")
    props = scene.get("props") or []
    print(f"props: {len(props)} rows, row0 = {props[0] if props else None}")
    print()
    seen = set()
    for row in props:
        fid = row[0]
        if (fid >> 24) & 0xF != 2:
            continue
        index = fid & 0xFFF
        if index in hits and index not in seen:
            seen.add(index)
            name, dirs, sides = hits[index]
            board, rnd = row[3], row[4]
            shape = "box" if board else ("drum" if rnd else "card-only")
            print(f"{name:24s} seen={row[15]} shape={shape} u={row[1]} v={row[2]} "
                  f"a={row[7]} b={row[8]} z0={row[9]} z1={row[10]} row={row}")


if __name__ == "__main__":
    main()
