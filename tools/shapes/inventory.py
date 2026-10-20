"""Every static thing on the Klamath maps, by its picture (FRM): how often it
stands, how the engine draws it today and where one instance is.

    py -3 tools/shapes/inventory.py --game-root ../fallout2-ce-3d/game [--scenes <mod-cache/scene>]

Reads the maps from the game and the exported scenes (scene.json) from the
mod cache; writes build/shapes-eval/inventory.json and prints the most
frequent FRMs.
"""
from __future__ import annotations

import argparse
import json
import sys
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "scene_export"))
sys.path.insert(0, str(ROOT / "tools" / "scene_export" / "vendor"))

from export import MAP_GROUPS, ReadOnlyGameFiles  # noqa: E402
from f2lib import MapFile  # noqa: E402

ITEM, SCENERY = 0, 2  # pid types of the things shapes are for


def todays_shapes(scene):
    """Object serial -> how the engine draws it now."""
    shape = {}
    boards = scene.get("boards") or []
    for row in boards:
        shape[row[-1]] = "card"
    for index in scene.get("crossed") or []:
        shape[boards[index][-1]] = "card (donor: crossed)"
    for index in scene.get("hidden") or []:
        shape[boards[index][-1]] = "part of a solid"
    for row in scene.get("props") or []:
        if row[3] >= 0:
            shape[boards[row[3]][-1]] = "drum" if row[4] else "box"
    for row in scene.get("walls") or []:
        shape.setdefault(row[8], "panel")
    for row in scene.get("doors") or []:
        shape[row[-1]] = "card (door)"
    for row in scene.get("decals") or []:
        shape.setdefault(row[-1], "flat")
    return shape


def inventory(game_root, scenes):
    gf = ReadOnlyGameFiles(game_root)
    things = defaultdict(lambda: {"count": 0, "today": Counter(), "instances": [], "name": ""})
    try:
        for name in MAP_GROUPS["klamath"]:
            game_map = MapFile.load(name, gf)
            for elevation in sorted(game_map.tiles):
                path = scenes / name / str(elevation) / "scene.json"
                if not path.is_file():
                    continue
                shape = todays_shapes(json.loads(path.read_text(encoding="utf-8")))
                for serial, obj in enumerate(game_map.objects[elevation]):
                    if obj.obj_type not in (ITEM, SCENERY) or obj.tile < 0:
                        continue
                    frm = (gf.art.name(obj.fid) or f"fid {obj.fid:08X}").lower()
                    thing = things[frm]
                    thing["count"] += 1
                    thing["name"] = thing["name"] or gf.protos.name(obj.pid) or ""
                    thing["today"][shape.get(serial, "not drawn")] += 1
                    if len(thing["instances"]) < 3:
                        thing["instances"].append([name, elevation, obj.tile, serial])
    finally:
        gf.close()
    # What the engine never draws (blockers, scroll eggs) has no shape.
    rows = [{"frm": frm, "name": t["name"], "count": t["count"], "today": dict(t["today"].most_common()),
             "instances": t["instances"]}
            for frm, t in things.items() if set(t["today"]) != {"not drawn"}]
    rows.sort(key=lambda row: (-row["count"], row["frm"]))
    return rows


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--game-root", type=Path, default=ROOT / "game")
    parser.add_argument("--scenes", type=Path, help="exported scenes (default: <game-root>/mod-cache/scene)")
    parser.add_argument("--out", type=Path, default=ROOT / "build" / "shapes-eval" / "inventory.json")
    parser.add_argument("--top", type=int, default=60)
    args = parser.parse_args(argv)
    rows = inventory(args.game_root, args.scenes or args.game_root / "mod-cache" / "scene")
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(rows, indent=1), encoding="utf-8")
    total = sum(row["count"] for row in rows)
    running = 0
    for row in rows[:args.top]:
        running += row["count"]
        today = ", ".join(f"{k} {v}" for k, v in row["today"].items())
        print(f"{row['count']:5} {100 * running / total:5.1f}%  {row['frm']:13} {row['name'][:18]:18} {today}  "
              f"{row['instances'][0]}")
    print(f"{len(rows)} FRMs, {total} things; {args.out}")


if __name__ == "__main__":
    main()
