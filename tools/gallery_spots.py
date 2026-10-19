"""Views to compare 2D and 3D on: every building of a map (by its roof) and a
grid over the floor where there are no roofs, as a --dev-map-tour list.

py -3 tools/gallery_spots.py --game-root ./game --maps kladwtwn,klatrap
"""

import argparse
import base64
import json
import sys
from pathlib import Path

GRID = 100
SPACING = 20  # squares between views of one map: about one screen apart
MAX_VIEWS = 6  # per map elevation: buildings first, then the floor grid


def square_to_tile(index):
    """The hex at a square's centre, as the engine counts it (game_mouse.cc)."""
    row, col = divmod(index, GRID)
    return 200 * (2 * row + 1) + 2 * col + 1


def roof_patches(roof):
    """Connected roof squares, 4-way, as tile_fill_roof floods them."""
    seen = [False] * len(roof)
    patches = []
    for start, value in enumerate(roof):
        if value == 0 or seen[start]:
            continue
        stack, patch = [start], []
        seen[start] = True
        while stack:
            index = stack.pop()
            patch.append(index)
            row, col = divmod(index, GRID)
            for r, c in ((row - 1, col), (row + 1, col), (row, col - 1), (row, col + 1)):
                if 0 <= r < GRID and 0 <= c < GRID:
                    other = r * GRID + c
                    if roof[other] and not seen[other]:
                        seen[other] = True
                        stack.append(other)
        patches.append(patch)
    return patches


def spots(scene):
    floor = [int(v) for v in memoryview(base64.b64decode(scene["floor"])).cast("H")]
    roof = [int(v) for v in memoryview(base64.b64decode(scene["roof"])).cast("H")]
    centres = []
    for patch in roof_patches(roof):
        if len(patch) >= 6:
            rows = sorted(index // GRID for index in patch)
            cols = sorted(index % GRID for index in patch)
            centres.append(rows[len(rows) // 2] * GRID + cols[len(cols) // 2])
    floored = [index for index, value in enumerate(floor) if value]
    if floored:
        rows = [index // GRID for index in floored]
        cols = [index % GRID for index in floored]
        for row in range(min(rows), max(rows) + 1, SPACING):
            for col in range(min(cols), max(cols) + 1, SPACING):
                if floor[row * GRID + col]:
                    centres.append(row * GRID + col)
    chosen = []
    for index in centres:
        row, col = divmod(index, GRID)
        if all(abs(row - r) + abs(col - c) >= SPACING // 2 for r, c in (divmod(i, GRID) for i in chosen)):
            chosen.append(index)
    return [square_to_tile(index) for index in chosen[:MAX_VIEWS]]


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-root", type=Path, required=True)
    parser.add_argument("--maps", required=True, help="comma-separated map names")
    args = parser.parse_args(argv)
    stops = []
    for name in args.maps.split(","):
        base = args.game_root / "mod-cache" / "scene" / name.strip()
        for directory in sorted(base.iterdir()) if base.is_dir() else []:
            scene = json.loads((directory / "scene.json").read_text(encoding="utf-8"))
            stops.extend(f"{name.strip()}:{scene['elevation']}:{tile}" for tile in spots(scene))
    print(",".join(stops))
    return 0


if __name__ == "__main__":
    sys.exit(main())
