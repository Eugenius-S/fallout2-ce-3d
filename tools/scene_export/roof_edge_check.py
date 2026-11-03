"""Checks that every roof edge stands (task 129): flat wall-top texels left and band geometry.

For each map elevation of a scene cache this reports

- flat: the dark ragged texels (grey concrete and rebar, in blobs of BLOB texels
  or more, at the cut edge of the roof's texture or off its square) that the
  free camera still draws lying flat on the roof within DEPTH of its boundary,
  and the number of squares that hold them;
- geometry: for every band, how far its plane is from the outer face of its wall
  (tiles, 0 is flush), and at every band end next to a perpendicular band the
  gap (the end short of that band's face) and the overshoot (the end past it).

    py -3 tools/scene_export/roof_edge_check.py --cache-root <cache> [--maps klamath]
"""

import argparse
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "vendor"))

from floor_sidecar import CELL_MARGIN, CELL_OUTER_SIZE, CELL_SIZE, decode_floor_sidecar  # noqa: E402
from wall_sidecar import wall_slabs  # noqa: E402
from roof_fringe import EDGE_STEPS, outward, GREY_TINT, _blobs  # noqa: E402

DARK = 30.0   # a texel darker than this (mean of the colours) and grey is concrete or rebar; the metal roofs' shading dither is lighter
BLOB = 8      # connected ragged dark texels that make a smear
DEPTH = 0.7   # from a boundary edge, in tiles
RIM = 0.05    # the square's own rim, jagged in every tile's art: a clear texel there is no hole
HOLE_REACH = 2  # a dark texel this near a clear texel of the square is ragged, in texels
DEEP = 6        # squares beyond a wall that tell which side is the building's outside (roof_fringe.OUTWARD_DEPTH)
WALL_NEAR = 0.3  # a wall body this near a band's plane belongs to its wall, in tiles
OFF_SQUARE = 3  # a dark texel this far off the square is ragged, in texels


def ragged_dark(cell, edges):
    """Mask of the dark ragged texels of one roof square's cell (rgba, CELL_OUTER_SIZE square) near its boundary edges."""
    rgb = cell[..., :3].astype(float)
    opaque = cell[..., 3] > 0
    grey = opaque & (rgb.mean(axis=2) < DARK) & (np.abs(rgb[..., 0] - rgb[..., 2]) < GREY_TINT)
    if not edges or not grey.any():
        return np.zeros_like(grey)
    t = (np.arange(CELL_OUTER_SIZE) + 0.5 - CELL_MARGIN) / CELL_SIZE
    v, u = np.meshgrid(t, t, indexing="ij")
    on = (u >= 0) & (u < 1) & (v >= 0) & (v < 1)
    clear = ~opaque & on & (u > RIM) & (u < 1 - RIM) & (v > RIM) & (v < 1 - RIM)
    padded = np.pad(clear, HOLE_REACH)
    near_hole = np.zeros_like(clear)
    for dy in range(-HOLE_REACH, HOLE_REACH + 1):
        for dx in range(-HOLE_REACH, HOLE_REACH + 1):
            near_hole |= padded[HOLE_REACH + dy:HOLE_REACH + dy + clear.shape[0],
                                HOLE_REACH + dx:HOLE_REACH + dx + clear.shape[1]]
    off = OFF_SQUARE / CELL_SIZE
    off_square = (u < -off) | (u >= 1 + off) | (v < -off) | (v >= 1 + off)
    dist = np.stack([v, 1.0 - v, u, 1.0 - u])
    depth = dist[list(edges)].min(axis=0)
    candidate = grey & (near_hole | off_square) & (depth < DEPTH)
    mask = np.zeros_like(candidate)
    for rows, cols in _blobs(candidate):
        if len(rows) >= BLOB:
            mask[rows, cols] = True
    return mask


def run_squares(entry):
    edge, plane, start, count = entry[:4]
    fixed = plane - (1 if edge in (1, 3) else 0)
    return [(fixed * 100 + a if edge < 2 else a * 100 + fixed) for a in range(start, start + count)]


def band_stats(entries, slabs=(), footprint=frozenset()):
    """How far each band stands from the outer face of its wall, and the gap and overshoot at its ends beside a band across it.

    off_face: the distance from the band's plane to the outer face of the wall
    bodies (slabs) along its span within WALL_NEAR of it, the outer one being the
    side with fewer roof squares in the DEEP squares beyond (the building's
    outside): 0 is flush with the wall's outer face. A band with no wall is not counted.
    """
    insets, gaps, overshoots = [], [], []
    for edge, plane, start, count, *_, face, top, lo, hi in entries:
        axis = 0 if edge < 2 else 1
        near = [slab.band() for slab in slabs
                if slab.axis == axis and slab.hi > lo + 0.5 and slab.lo < hi - 0.5
                and slab.band()[0] - WALL_NEAR <= face <= slab.band()[1] + WALL_NEAR]
        if near:
            low, high = min(low for low, _ in near), max(high for _, high in near)
            side = outward(footprint, edge, low, high, start, count, DEEP)
            insets.append(abs(face - (high if side > 0 else low)))
    for n, a in enumerate(entries):
        for m, b in enumerate(entries):
            if n == m or (a[0] < 2) == (b[0] < 2):
                continue
            b_end = b[2] + b[3]
            for end_line, mine, sign in ((a[2], a[-2], -1.0), (a[2] + a[3], a[-1], 1.0)):
                if b[1] == end_line and a[1] in (b[2], b_end) and (b[0] in (0, 2)) == (sign < 0):
                    excess = (mine - b[-4]) * sign
                    (overshoots if excess > 1e-6 else gaps).append(abs(excess))
    return {"bands": len(entries), "off_face_max": max(insets, default=0.0),
            "off_face_mean": float(np.mean(insets)) if insets else 0.0, "gap_max": max(gaps, default=0.0),
            "overshoot_max": max(overshoots, default=0.0), "corners": len(gaps) + len(overshoots)}


def measure(directory):
    """(flat texels, squares with them, band stats) of one scene cache directory."""
    sidecar = decode_floor_sidecar((directory / "floor.bin").read_bytes())
    tiles = np.asarray(Image.open(directory / "tiles.png").convert("RGBA"))
    fringe_png = directory / "roof_fringe.png"
    fringe = np.asarray(Image.open(fringe_png).convert("RGBA")) if fringe_png.exists() else None
    roof = sidecar["roof"]
    cells = sidecar["cells"]
    banded = {square for entry in sidecar["fringe"] for square in run_squares(entry)}
    patches = {index: (x, y) for index, x, y in sidecar.get("patches", [])}
    footprint = {index for index, cell in enumerate(roof) if cell}
    texels = squares = 0
    for index in sorted(footprint):
        edges = [edge for edge, (dx, dz) in enumerate(EDGE_STEPS)
                 if not (0 <= index % 100 + dx < 100 and 0 <= index // 100 + dz < 100)
                 or (index // 100 + dz) * 100 + index % 100 + dx not in footprint]
        if not edges:
            continue
        if index in patches:
            x, y = patches[index]
            cell = fringe[y:y + CELL_OUTER_SIZE, x:x + CELL_OUTER_SIZE]
        elif index in banded:
            continue
        else:
            x, y = cells[roof[index] - 1]
            cell = tiles[y:y + CELL_OUTER_SIZE, x:x + CELL_OUTER_SIZE]
        count = int(ragged_dark(cell, edges).sum())
        if count:
            texels += count
            squares += 1
    scene = json.loads((directory / "scene.json").read_text(encoding="utf-8"))
    return texels, squares, band_stats(sidecar["fringe"], wall_slabs(scene), footprint)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache-root", type=Path, required=True)
    parser.add_argument("--maps", default="kladwtwn,klatrap,klacanyn,klagraz,klamall,klaratcv,klatoxcv")
    args = parser.parse_args(argv)
    total = [0, 0]
    for name in args.maps.split(","):
        for directory in sorted((args.cache_root / name).glob("[0-9]")):
            if not (directory / "floor.bin").is_file():
                continue
            texels, squares, bands = measure(directory)
            total[0] += texels
            total[1] += squares
            print(json.dumps({"map": name, "elevation": directory.name, "flat_texels": texels,
                              "flat_squares": squares,
                              **{k: round(v, 3) if isinstance(v, float) else v for k, v in bands.items()}}))
    print(json.dumps({"total_flat_texels": total[0], "total_flat_squares": total[1]}))


if __name__ == "__main__":
    main()
