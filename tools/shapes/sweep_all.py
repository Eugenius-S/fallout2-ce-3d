"""Build every map elevation of the game in process and report (task 120's
whole-game check that a pipeline change builds everywhere).

    py -3 tools/shapes/sweep_all.py [out.txt]

Prints "map eN: OK props=... rests=..." per elevation, or the exception.
"""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "scene_export"))
sys.path.insert(0, str(ROOT / "tools" / "scene_export" / "vendor"))

from export import ReadOnlyGameFiles  # noqa: E402
from fp.scene import SceneBuilder  # noqa: E402


def main():
    out = open(sys.argv[1], "w", encoding="utf-8") if len(sys.argv) > 1 else sys.stdout
    gf = ReadOnlyGameFiles(ROOT / "game")
    maps = sorted(name[5:-4].lower() for name in gf.glob("maps/*.map"))
    print(f"{len(maps)} map files seen", file=out)
    ok = bad = 0
    try:
        for map_name in maps:
            for elevation in (0, 1, 2):
                try:
                    builder = SceneBuilder(gf, map_name, elevation)
                    builder.build_tiles()
                    builder.build_objects()
                    print(f"{map_name} e{elevation}: OK props={len(builder.props)} rests={len(builder.rests)}", file=out)
                    ok += 1
                except (Exception, SystemExit) as error:     # noqa: BLE001 - the sweep reports, not raises
                    print(f"{map_name} e{elevation}: {type(error).__name__}: {error}", file=out)
                    bad += 1
    finally:
        gf.close()
        print(f"done: {ok} ok, {bad} failed", file=out)
    if out is not sys.stdout:
        out.close()


if __name__ == "__main__":
    main()
