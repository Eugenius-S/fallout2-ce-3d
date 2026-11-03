"""Checks that every roof edge stands (task 129): flat wall-top texels left and band geometry.

For each map elevation of a scene cache this reports

- flat: the dark ragged texels (grey concrete and rebar, in blobs of BLOB texels
  or more, at the cut edge of the roof's texture or off its square) that the
  free camera still draws lying flat on the roof within DEPTH of its boundary,
  and the number of squares that hold them;
- flat_slab_squares: boundary squares whose drawn roof still has a strip of
  dark grey (half of it or more, no blob or hole rule, a looser test than the
  exporter's) along an edge with no roof beyond it: a wall top lying flat;
- lost_squares: roof squares whose original tile is painted whole (WHOLE_COVER of
  the square) but whose flat cover in flight (the patch cell; 0 when a band
  crosses it with no patch, or the patch is blank) is lower than the tile's by
  more than the slab strips of its open edges (SLAB_STRIP + RIM each) and a
  small slack: holes in the roof;
- lane_squares: banded roof squares whose flat cell leaves more than LANE_CLEAR of the
  inner side of the band's plane clear (the side toward the rest of the roof; a square
  crossed by two bands counts the part inside both): you look down into the building
  through the lane. spill_squares: banded squares whose cell paints more than
  LANE_CLEAR of the outer side: roof sticking out past the band;
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
from roof_fringe import DONOR_COVER, inner_edge, tidy_cell, EDGE_STEPS, components_of, outward, GREY_TINT, _blobs, _dilate  # noqa: E402

VERBOSE = False
DARK = 30.0   # a texel darker than this (mean of the colours) and grey is concrete or rebar; the metal roofs' shading dither is lighter
BLOB = 8      # connected ragged dark texels that make a smear
DEPTH = 0.7   # from a boundary edge, in tiles
RIM_STRIP = 0.07  # a slab strip's rim (roof_fringe.RIM), in lost_squares
RIM = 0.05    # the square's own rim, jagged in every tile's art: a clear texel there is no hole
HOLE_REACH = 2  # a dark texel this near a clear texel of the square is ragged, in texels
DEEP = 6        # squares beyond a wall that tell which side is the building's outside (roof_fringe.OUTWARD_DEPTH)
WALL_NEAR = 0.3  # a wall body this near a band's plane belongs to its wall, in tiles
CORNER_NEAR = 0.7  # two bands make a corner when their ends and faces lie this near each other, in tiles
WALL_SHARE = 0.03  # a wall slab covers a square when it reaches this far into it, in tiles
WHOLE_COVER = 0.9  # of the square painted: a whole roof square (roof_fringe.WHOLE_COVER)
LOST_SLACK = 0.1   # cover a whole square may lose beyond its slab strips (the cut's ragged rim, RAG_REACH texels) and still not be a hole
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


def flat_slab(cell, edges, plain=None):
    """Open edges of a drawn roof square whose strip (SLAB_STRIP) is mostly dark grey: a wall top lying flat.

    plain: mask of the cell's texels that are the roof's own texture (equal to the texel of a tile that stands in
    the roof's inside, see plain_mask): a dark roof (klamall's) is no wall top.
    """
    rgb = cell[..., :3].astype(float)
    opaque = (cell[..., 3] > 0) & (True if plain is None else ~plain)
    grey = opaque & (rgb.mean(axis=2) < SLAB_LUMA) & (np.abs(rgb[..., 0] - rgb[..., 2]) < GREY_TINT)
    t = (np.arange(CELL_OUTER_SIZE) + 0.5 - CELL_MARGIN) / CELL_SIZE
    v, u = np.meshgrid(t, t, indexing="ij")
    on = (u >= 0) & (u < 1) & (v >= 0) & (v < 1)
    dist = np.stack([v, 1.0 - v, u, 1.0 - u])
    return [edge for edge in edges
            if (grey & on & (dist[edge] < SLAB_STRIP)).sum() >= SLAB_HALF * (on & (dist[edge] < SLAB_STRIP)).sum()]


def plain_mask(cell, interior):
    """Mask of the texels of a cell that equal the same texel of one of the interior tiles' cells (the roof's own texture)."""
    mask = np.zeros(cell.shape[:2], bool)
    for tile in interior:
        mask |= (cell == tile).all(axis=2) & (cell[..., 3] > 0)
    return mask


def cover_of(cell):
    """The share of the square (u, v in 0..1) that a cell (rgba, CELL_OUTER_SIZE square) paints."""
    t = (np.arange(CELL_OUTER_SIZE) + 0.5 - CELL_MARGIN) / CELL_SIZE
    v, u = np.meshgrid(t, t, indexing="ij")
    on = (u >= 0) & (u < 1) & (v >= 0) & (v < 1)
    return float(((cell[..., 3] > 0) & on).sum() / on.sum())


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


LANE_CLEAR = 0.05  # of a band's inner side a flat cell may leave clear (the cut's ragged rim, a texel or two of dither)
LANE_MIN = 0.02    # of the square: a band whose inner side is less is no lane to look through


def lane_stats(entries, slabs, footprint, patches, fringe, plain_roofs=frozenset()):
    """(lane_squares, spill_squares, lanes, no_donor, outside): see the module's text.

    lanes: [(square index, clear share)]; plain_roofs: the squares that stand on a plain roof tile (painted whole). A
    lane in a roof with no plain square has no donor to fill it from, and a square with no roof behind its band is a ring round an open courtyard (both no_donor, not counted in lane_squares). outside: {square index: the share of the square on the outer side of its bands}.
    """
    planes = {}
    for edge, plane, start, count, *_, face, top, lo, hi in entries:
        axis = 0 if edge < 2 else 1
        near = [slab.band() for slab in slabs
                if slab.axis == axis and slab.hi > lo + 0.5 and slab.lo < hi - 0.5
                and slab.band()[0] - WALL_NEAR <= face <= slab.band()[1] + WALL_NEAR]
        low, high = (min(b[0] for b in near), max(b[1] for b in near)) if near else (face, face)
        side = outward(footprint, edge, low, high, start, count, DEEP)
        for index in run_squares((edge, plane, start, count)):
            base = index // 100 if axis == 0 else index % 100
            planes.setdefault(index, []).append((axis, face - base, side))
    t = (np.arange(CELL_OUTER_SIZE) + 0.5 - CELL_MARGIN) / CELL_SIZE
    v, u = np.meshgrid(t, t, indexing="ij")
    on = (u >= 0) & (u < 1) & (v >= 0) & (v < 1)
    lanes, spills, no_donor, outside = [], 0, [], {}
    comp = components_of(footprint)
    # the exporter's donor is a plain tile no band crosses (donor_tiles): a roof whose only plain squares carry bands has none
    donorless = set(comp.values()) - {comp[index] for index in plain_roofs if index in comp and index not in planes}
    # the exporter's roofed squares (write_fringe_art): plain roof, or a roofed square, behind each band
    def behind(index):
        return [(index // 100 + EDGE_STEPS[edge][1]) * 100 + index % 100 + EDGE_STEPS[edge][0]
                for edge in {inner_edge(clip) for clip in planes[index]}]

    candidates = {index for index in planes if all(back in footprint for back in behind(index))}
    roofed = set()
    while True:   # grown out from the plain roof: strips facing each other never vouch for each other
        grown = {index for index in candidates - roofed
                 if all((back in plain_roofs and back not in planes) or back in roofed for back in behind(index))}
        if not grown:
            break
        roofed |= grown
    for index, found in sorted(planes.items()):
        inner = on.copy()
        for axis, face, side in found:
            coord = v if axis == 0 else u
            inner &= (coord < face) if side > 0 else (coord >= face)
        if index in patches:
            x, y = patches[index]
            painted = fringe[y:y + CELL_OUTER_SIZE, x:x + CELL_OUTER_SIZE, 3] > 0
        else:
            painted = np.zeros_like(on)
        around = np.pad(painted, 1)
        dither = ~painted & around[:-2, 1:-1] & around[2:, 1:-1] & around[1:-1, :-2] & around[1:-1, 2:]
        if inner.sum() >= LANE_MIN * on.sum():
            # the square's own jagged rim (RIM) is clear in every roof tile: no lane
            core = (u > RIM) & (u < 1 - RIM) & (v > RIM) & (v < 1 - RIM)
            clear = float((inner & core & ~painted & ~dither).sum() / inner.sum())
            if clear > LANE_CLEAR:
                # a square with no plain roof behind its band (a ring of wall tops round an open ruin) is no lane either
                (no_donor if comp.get(index) in donorless or index not in roofed else lanes).append((index, round(clear, 2)))
        outer = on & ~inner
        outside[index] = float(outer.sum() / on.sum())
        if outer.sum() and (outer & painted).sum() / outer.sum() > LANE_CLEAR:
            spills += 1
    return len(lanes), spills, lanes, no_donor, outside


def measure(directory):
    """(flat texels, squares with them, flat slab squares, flat grey texels, lost squares, band stats) of one scene cache directory."""
    sidecar = decode_floor_sidecar((directory / "floor.bin").read_bytes())
    tiles = np.asarray(Image.open(directory / "tiles.png").convert("RGBA"))
    fringe_png = directory / "roof_fringe.png"
    fringe = np.asarray(Image.open(fringe_png).convert("RGBA")) if fringe_png.exists() else None
    roof = sidecar["roof"]
    cells = sidecar["cells"]
    banded = {square for entry in sidecar["fringe"] for square in run_squares(entry)}
    patches = {index: (x, y) for index, x, y in sidecar.get("patches", [])}
    footprint = {index for index, cell in enumerate(roof) if cell}
    texels = squares = slabs = grey_texels = lost = 0
    plain_roofs = set()
    for index in footprint:
        x, y = cells[roof[index] - 1]
        # plain as the exporter's donor_tiles judges it: its tidied cell painted DONOR_COVER or more
        if cover_of(tidy_cell(tiles[y:y + CELL_OUTER_SIZE, x:x + CELL_OUTER_SIZE], ())) >= DONOR_COVER:
            plain_roofs.add(index)
    wall = wall_slabs(json.loads((directory / "scene.json").read_text(encoding="utf-8")))
    lane, spill, lanes, no_donor, outside = lane_stats(sidecar["fringe"], wall, footprint, patches, fringe, plain_roofs)
    # each roof's own plain tiles (its interior ones and those a donor fill can come from): a dark roof exempts only
    # its own texels, never a slab on another building
    comp = components_of(footprint)
    interior_tiles = {}
    for index in footprint:
        if (index in plain_roofs and index not in banded) or all(0 <= index % 100 + dx < 100 and 0 <= index // 100 + dz < 100
               and (index // 100 + dz) * 100 + index % 100 + dx in footprint for dx, dz in EDGE_STEPS):
            interior_tiles.setdefault(comp[index], set()).add(roof[index])
    interior_cells = {number: [tiles[cells[tile - 1][1]:cells[tile - 1][1] + CELL_OUTER_SIZE,
                                     cells[tile - 1][0]:cells[tile - 1][0] + CELL_OUTER_SIZE] for tile in sorted(found)]
                      for number, found in interior_tiles.items()}
    for index in sorted(footprint):
        edges = [edge for edge, (dx, dz) in enumerate(EDGE_STEPS)
                 if not (0 <= index % 100 + dx < 100 and 0 <= index // 100 + dz < 100)
                 or (index // 100 + dz) * 100 + index % 100 + dx not in footprint]
        x, y = cells[roof[index] - 1]
        original = cover_of(tiles[y:y + CELL_OUTER_SIZE, x:x + CELL_OUTER_SIZE])
        if original >= WHOLE_COVER:
            if index in patches:
                px, py = patches[index]
                flight = cover_of(fringe[py:py + CELL_OUTER_SIZE, px:px + CELL_OUTER_SIZE])
            else:
                flight = 0.0 if index in banded else original
            if flight < original * (1 - outside.get(index, 0.0)) - (SLAB_STRIP + RIM_STRIP) * len(edges) - LOST_SLACK:
                lost += 1
                if VERBOSE:
                    print("lost square:", index, round(original, 2), round(flight, 2))
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
        if flat_slab(cell, edges, plain_mask(cell, interior_cells.get(comp[index], []))):
            slabs += 1
            if VERBOSE:
                print("flat slab square:", index)
        grey_texels += int(ragged_dark(cell, edges).sum())
        # a patch is drawn flat whatever its edges: every edge counts (an interior partition's cut too)
        count = int(ragged_thin(cell, [0, 1, 2, 3] if index in patches else edges).sum())
        if count:
            texels += count
            squares += 1
    stats = band_stats(sidecar["fringe"], wall, footprint)
    stats.update({"lane_squares": lane, "spill_squares": spill, "lane_no_donor": len(no_donor)})
    if VERBOSE:
        print("lanes:", " ".join(f"{i}:{c}" for i, c in lanes), "| no donor:", " ".join(f"{i}:{c}" for i, c in no_donor))
    return texels, squares, slabs, grey_texels, lost, stats


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache-root", type=Path, required=True)
    parser.add_argument("--maps", default="kladwtwn,klatrap,klacanyn,klagraz,klamall,klaratcv,klatoxcv")
    parser.add_argument("--lanes", action="store_true", help="list the lane squares")
    args = parser.parse_args(argv)
    global VERBOSE
    VERBOSE = args.lanes
    total = [0, 0, 0, 0]
    for name in args.maps.split(","):
        for directory in sorted((args.cache_root / name).glob("[0-9]")):
            if not (directory / "floor.bin").is_file():
                continue
            texels, squares, slab_squares, grey, lost, bands = measure(directory)
            total[0] += texels
            total[1] += squares
            total[2] += slab_squares
            total[3] += lost
            print(json.dumps({"map": name, "elevation": directory.name, "flat_texels": texels,
                              "flat_squares": squares, "flat_grey_texels": grey, "flat_slab_squares": slab_squares, "lost_squares": lost,
                              **{k: round(v, 3) if isinstance(v, float) else v for k, v in bands.items()}}))
    print(json.dumps({"total_flat_texels": total[0], "total_flat_squares": total[1], "total_flat_slab_squares": total[2], "total_lost_squares": total[3]}))


if __name__ == "__main__":
    main()
