"""Checks that every roof edge stands (task 129): flat wall-top texels left and band geometry.

For each map elevation of a scene cache this reports

- flat: the dark ragged texels (grey concrete and rebar, in blobs of BLOB texels
  or more, at the cut edge of the roof's texture or off its square) that the
  free camera still draws lying flat on the roof within DEPTH of its boundary,
  and the number of squares that hold them;
- flat_slab_squares: boundary squares whose drawn roof still has a strip of
  dark grey (half of it or more, no blob or hole rule, a looser test than the
  exporter's) along an edge with no roof beyond it: a wall top lying flat;
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
from roof_fringe import EDGE_STEPS, outward, GREY_TINT, _blobs, _dilate  # noqa: E402

DARK = 30.0   # a texel darker than this (mean of the colours) and grey is concrete or rebar; the metal roofs' shading dither is lighter
BLOB = 8      # connected ragged dark texels that make a smear
DEPTH = 0.7   # from a boundary edge, in tiles
RIM = 0.05    # the square's own rim, jagged in every tile's art: a clear texel there is no hole
HOLE_REACH = 2  # a dark texel this near a clear texel of the square is ragged, in texels
DEEP = 6        # squares beyond a wall that tell which side is the building's outside (roof_fringe.OUTWARD_DEPTH)
WALL_NEAR = 0.3  # a wall body this near a band's plane belongs to its wall, in tiles
CORNER_NEAR = 0.7  # two bands make a corner when their ends and faces lie this near each other, in tiles
WALL_SHARE = 0.03  # a wall slab covers a square when it reaches this far into it, in tiles
OFF_SQUARE = 3  # a dark texel this far off the square is ragged, in texels


ANY_DARK = 60.0  # in ragged_thin: any hue this dark (black, grey concrete, dark brown) counts
THIN = 3         # in ragged_thin: painted structures narrower than 2 * THIN + 1 texels are ragged art
SOLID = 7        # in ragged_thin: painted texels among the 9 of its neighbourhood that make a texel solid art, not dither
EDGE_BAND = 0.08  # in ragged_thin: the square's own jagged rim (tile art overhangs it) is no ragged art


def _erode(mask, reach):
    return ~_dilate(~mask, reach)


def ragged_thin(cell, edges):
    """Mask of the dark ragged texels a flat roof cell draws, told by shape alone (no colour classifier, no wall list).

    A wall top's ragged edge is fingers, rebar stumps and chunks: painted
    structures thinner than 2 * THIN + 1 texels (they vanish under a morphological opening
    of the painted mask) or loose blobs; whatever their hue (black, grey,
    dark brown) when they are dark (mean below ANY_DARK). The square's own rim
    (EDGE_BAND) and texels more than DEPTH from every edge in `edges` are left
    out. A smooth cut of solid roof is no ragged art.
    """
    painted = cell[..., 3] > 0
    rgb = cell[..., :3].astype(float)
    t = (np.arange(CELL_OUTER_SIZE) + 0.5 - CELL_MARGIN) / CELL_SIZE
    v, u = np.meshgrid(t, t, indexing="ij")
    inner = (u >= EDGE_BAND) & (u < 1 - EDGE_BAND) & (v >= EDGE_BAND) & (v < 1 - EDGE_BAND)
    # the cell is cut out of art that goes on beyond it: grow the borders before opening
    grown = np.pad(painted, THIN, mode="edge")
    opened = _dilate(_erode(grown, THIN), THIN)[THIN:-THIN, THIN:-THIN]
    dist = np.stack([v, 1.0 - v, u, 1.0 - u])
    depth = dist[list(edges)].min(axis=0) if edges else np.full_like(u, 9.0)
    # a checkerboard of painted and clear texels is the roof's shading dither, not wall top: the art of a finger
    # is solid, so most of the texel's 3 x 3 neighbourhood is painted
    padded = np.pad(painted, 1).astype(int)
    solid = sum(padded[1 + dy:padded.shape[0] - 1 + dy, 1 + dx:padded.shape[1] - 1 + dx]
                for dy in (-1, 0, 1) for dx in (-1, 0, 1)) >= SOLID
    candidate = painted & ~opened & solid & (rgb.mean(axis=2) < ANY_DARK) & inner & (depth < DEPTH)
    mask = np.zeros_like(candidate)
    for rows, cols in _blobs(candidate):
        if len(rows) >= BLOB:
            mask[rows, cols] = True
    return mask


def ragged_dark(cell, edges):
    """Mask of the dark grey texels of one roof square's cell (rgba, CELL_OUTER_SIZE square) near its boundary edges,
    ragged by holes: the first, narrow count (flat_grey_texels), kept for comparison with older runs."""
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


SLAB_LUMA = 55.0   # a looser grey than the exporter's classifier asks nothing of blobs, holes or shape:
SLAB_STRIP = 0.35  # the drawn roof square's strip this wide along an open edge ...
SLAB_HALF = 0.5    # ... that is half dark grey or more is concrete lying flat (the exporter stands up 0.85)


def flat_slab(cell, edges):
    """Open edges of a drawn roof square whose strip (SLAB_STRIP) is mostly dark grey: a wall top lying flat."""
    rgb = cell[..., :3].astype(float)
    opaque = cell[..., 3] > 0
    grey = opaque & (rgb.mean(axis=2) < SLAB_LUMA) & (np.abs(rgb[..., 0] - rgb[..., 2]) < GREY_TINT)
    t = (np.arange(CELL_OUTER_SIZE) + 0.5 - CELL_MARGIN) / CELL_SIZE
    v, u = np.meshgrid(t, t, indexing="ij")
    on = (u >= 0) & (u < 1) & (v >= 0) & (v < 1)
    dist = np.stack([v, 1.0 - v, u, 1.0 - u])
    return [edge for edge in edges
            if (grey & on & (dist[edge] < SLAB_STRIP)).sum() >= SLAB_HALF * (on & (dist[edge] < SLAB_STRIP)).sum()]


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
    no_wall = 0
    for edge, plane, start, count, *_, face, top, lo, hi in entries:
        axis = 0 if edge < 2 else 1
        # stretches of the band's span (whole squares) with no wall slab under them: floating over open ground
        held = [any(slab.axis == axis and slab.hi > a + WALL_SHARE and slab.lo < a + 1 - WALL_SHARE
                    and slab.band()[0] - WALL_NEAR <= face <= slab.band()[1] + WALL_NEAR for slab in slabs)
                for a in range(start, start + count)]
        no_wall += sum(1 for n, ok in enumerate(held) if not ok and (n == 0 or held[n - 1]))
        near = [slab.band() for slab in slabs
                if slab.axis == axis and slab.hi > lo + 0.5 and slab.lo < hi - 0.5
                and slab.band()[0] - WALL_NEAR <= face <= slab.band()[1] + WALL_NEAR]
        if near:
            low, high = min(low for low, _ in near), max(high for _, high in near)
            side = outward(footprint, edge, low, high, start, count, DEEP)
            insets.append(abs(face - (high if side > 0 else low)))
    # corners, by geometry: a band's end has a corner with the nearest band across it whose face lies
    # within CORNER_NEAR of the end, and whose span reaches the band's own face
    tops = []
    for n, a in enumerate(entries):
        for end in (-2, -1):
            best = None
            for m, b in enumerate(entries):
                if n == m or (a[0] < 2) == (b[0] < 2):
                    continue
                gap = abs(a[end] - b[-4])
                if gap <= CORNER_NEAR and b[-2] - CORNER_NEAR <= a[-4] <= b[-1] + CORNER_NEAR and (best is None or gap < best[0]):
                    best = (gap, b)
            if best is not None:
                excess = (a[end] - best[1][-4]) * (-1.0 if end == -2 else 1.0)
                (overshoots if excess > 1e-6 else gaps).append(abs(excess))
                tops.append(abs(a[-3] - best[1][-3]))
    return {"bands": len(entries), "off_face_max": max(insets, default=0.0),
            "off_face_mean": float(np.mean(insets)) if insets else 0.0, "gap_max": max(gaps, default=0.0),
            "overshoot_max": max(overshoots, default=0.0), "corners": len(gaps) + len(overshoots),
            "corner_top_max": max(tops, default=0.0),
            "corner_off": sum(1 for g in gaps + overshoots if g > 1e-6), "no_wall_stretches": no_wall}


def measure(directory):
    """(flat texels, squares with them, flat slab squares, flat grey texels, band stats) of one scene cache directory."""
    sidecar = decode_floor_sidecar((directory / "floor.bin").read_bytes())
    tiles = np.asarray(Image.open(directory / "tiles.png").convert("RGBA"))
    fringe_png = directory / "roof_fringe.png"
    fringe = np.asarray(Image.open(fringe_png).convert("RGBA")) if fringe_png.exists() else None
    roof = sidecar["roof"]
    cells = sidecar["cells"]
    banded = {square for entry in sidecar["fringe"] for square in run_squares(entry)}
    patches = {index: (x, y) for index, x, y in sidecar.get("patches", [])}
    footprint = {index for index, cell in enumerate(roof) if cell}
    texels = squares = slabs = grey_texels = 0
    for index in sorted(footprint):
        edges = [edge for edge, (dx, dz) in enumerate(EDGE_STEPS)
                 if not (0 <= index % 100 + dx < 100 and 0 <= index // 100 + dz < 100)
                 or (index // 100 + dz) * 100 + index % 100 + dx not in footprint]
        if index in patches:
            x, y = patches[index]
            cell = fringe[y:y + CELL_OUTER_SIZE, x:x + CELL_OUTER_SIZE]
        elif not edges:
            continue
        elif index in banded:
            continue
        else:
            x, y = cells[roof[index] - 1]
            cell = tiles[y:y + CELL_OUTER_SIZE, x:x + CELL_OUTER_SIZE]
        slabs += bool(flat_slab(cell, edges))
        grey_texels += int(ragged_dark(cell, edges).sum())
        # a patch is drawn flat whatever its edges: every edge counts (an interior partition's cut too)
        count = int(ragged_thin(cell, [0, 1, 2, 3] if index in patches else edges).sum())
        if count:
            texels += count
            squares += 1
    scene = json.loads((directory / "scene.json").read_text(encoding="utf-8"))
    return texels, squares, slabs, grey_texels, band_stats(sidecar["fringe"], wall_slabs(scene), footprint)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache-root", type=Path, required=True)
    parser.add_argument("--maps", default="kladwtwn,klatrap,klacanyn,klagraz,klamall,klaratcv,klatoxcv")
    args = parser.parse_args(argv)
    total = [0, 0, 0]
    for name in args.maps.split(","):
        for directory in sorted((args.cache_root / name).glob("[0-9]")):
            if not (directory / "floor.bin").is_file():
                continue
            texels, squares, slab_squares, grey, bands = measure(directory)
            total[0] += texels
            total[1] += squares
            total[2] += slab_squares
            print(json.dumps({"map": name, "elevation": directory.name, "flat_texels": texels,
                              "flat_squares": squares, "flat_grey_texels": grey, "flat_slab_squares": slab_squares,
                              **{k: round(v, 3) if isinstance(v, float) else v for k, v in bands.items()}}))
    print(json.dumps({"total_flat_texels": total[0], "total_flat_squares": total[1], "total_flat_slab_squares": total[2]}))


if __name__ == "__main__":
    main()
