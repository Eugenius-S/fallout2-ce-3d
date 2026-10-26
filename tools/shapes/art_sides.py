"""How many sides (stored directions) each item and scenery FRM has.

    py -3 tools/shapes/art_sides.py --game-root ../fallout2-ce-3d/game

A side is a direction whose frames differ from the previous one's in the
file (art.cc keeps one copy of shared frames). Critters have 6; most items
and scenery have 1. Prints every item and scenery FRM with more than one side.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "scene_export"))
sys.path.insert(0, str(ROOT / "tools" / "scene_export" / "vendor"))

from export import ReadOnlyGameFiles  # noqa: E402
from f2lib.frm import Frm  # noqa: E402

TYPES = {0: "items", 2: "scenery"}


def sides(game_root):
    gf = ReadOnlyGameFiles(game_root)
    found = []
    try:
        for obj_type, folder in TYPES.items():
            for index, name in enumerate(gf.art_list(obj_type)):
                if not name or not name.lower().endswith(".frm"):
                    continue
                if not gf.exists(f"art/{folder}/{name}"):
                    continue
                data = gf.read(f"art/{folder}/{name}")
                frm = Frm.from_bytes(data)
                if frm.direction_count > 1:
                    found.append((folder, index, name.lower(), frm.direction_count, frm.direction_map))
    finally:
        gf.close()
    return found


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--game-root", type=Path, required=True)
    args = parser.parse_args()
    for folder, index, name, count, direction_map in sides(args.game_root):
        print(f"{folder}\t{index}\t{name}\t{count}\t{direction_map}")


if __name__ == "__main__":
    main()
