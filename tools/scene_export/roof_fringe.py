"""Roof tiles that are the broken tops of walls: ragged dark concrete along a roof's edge.

The game draws roofs 96 pixels above the floor. A roof's edge tile may carry
the broken top of the wall below it: a thin slab of concrete along one edge of
the tile with rebar and ragged concrete sticking up from it (task 129, find 6).
The exporter's top-down tile cell flattens that picture onto the roof plane,
where the rebar lies in streaks. These tiles are told by their art, no list:
less than half of the tile's diamond is painted, the painted pixels break up
along each row (ragged) and are dark (concrete, not a metal roof with a hole or
a solid trim strip).

Each such tile's art is split into one piece per wall edge it sits on (a
corner tile has two). The pieces of one wall line that follow each other are one
run, and a run's art is unrolled along the line into one strip, the way a wall's
art is unrolled along the wall: the picture is sheared vertically on the game's
screen until the wall line is horizontal, so the concrete lies along the bottom
of the strip and the rebar stands up from it. roof_fringe.png holds the strips;
the renderer stands each run up as a band on the wall below the roof edge,
in the plane of that wall's outer face, under the free camera (see roofs.cc).
The band is a belt, the wall's own art going on up (belt_art), with the
ragged edge on it: the strip's lowest rows (compose_band).
Each run is matched to its wall (wall_under) by a general rule; a run with no
wall under it is not exported and its squares stay flat.
"""

import numpy as np
from PIL import Image

MAX_COVER = 0.5   # of the tile's diamond painted
MAX_ROW_FILL = 0.9  # painted share of each row's span: solid strips fill it
MAX_LUMA = 50.0   # mean of the painted pixels: growth is dark


def is_fringe(rgba):
    """Is this iso tile art (h, w, 4) a roof fringe? See the module's text."""
    from fp import proj
    painted = rgba[..., 3] > 0
    if not painted.any():
        return False
    ys, xs = np.mgrid[0:painted.shape[0], 0:painted.shape[1]]
    u, v = proj.delta_to_ground(xs + 0.5 - 48, ys + 0.5)
    diamond = (u >= 0) & (u < 1) & (v >= 0) & (v < 1)
    if (painted & diamond).sum() / diamond.sum() >= MAX_COVER:
        return False
    spans = sum(int(np.flatnonzero(row)[-1] - np.flatnonzero(row)[0] + 1) for row in painted if row.any())
    if painted.sum() / spans >= MAX_ROW_FILL:
        return False
    return float(rgba[..., :3][painted].mean()) < MAX_LUMA


EDGE_V0, EDGE_V1, EDGE_U0, EDGE_U1 = 0, 1, 2, 3  # the tile's edge v=0, v=1, u=0, u=1
MIN_EDGE_SHARE = 0.3  # of the slab pixels: fewer is no wall edge
SLAB_DEPTH = 0.3  # a slab pixel lies this near an edge, in tiles
MIN_EDGE_PIXELS = 6


def edge_pieces(rgba):
    """Split a fringe tile's art by the wall edge each painted pixel belongs to.

    Returns [(edge, art)] where art is rgba with only that edge's pixels left.
    A pixel inside the tile's diamond belongs to its nearest edge; the edges
    holding a real share of those are the tile's wall edges. A pixel off the
    diamond (rebar, a loose chunk) takes the nearest wall edge.
    """
    from fp import proj
    painted = rgba[..., 3] > 0
    ys, xs = np.mgrid[0:painted.shape[0], 0:painted.shape[1]]
    u, v = proj.delta_to_ground(xs + 0.5 - 48, ys + 0.5)
    dist = np.stack([v, 1.0 - v, u, 1.0 - u])  # per edge: the ground distance inside from it
    nearest = np.abs(dist).argmin(axis=0)
    inside = painted & (u >= 0) & (u < 1) & (v >= 0) & (v < 1) & (np.abs(dist).min(axis=0) < SLAB_DEPTH)
    basis = inside if inside.any() else painted
    counts = [int((basis & (nearest == edge)).sum()) for edge in range(4)]
    wall = [edge for edge in range(4)
            if counts[edge] >= MIN_EDGE_PIXELS and counts[edge] >= MIN_EDGE_SHARE * basis.sum()]
    if not wall:
        wall = [int(np.argmax(counts))]
    pick = np.abs(dist[wall]).argmin(axis=0)
    pieces = []
    for n, edge in enumerate(wall):
        keep = painted & (pick == n)
        if not keep.any():
            continue
        art = rgba.copy()
        art[~keep] = 0
        pieces.append((edge, art))
    return pieces


STRIP_ROWS = 40  # a strip's height in art pixels, measured up from the concrete's lowest row


def _tile_screen(index):
    """Screen pixel (relative to the origin) of the art's top vertex of a roof square."""
    from fp import proj
    x, z = index % 100, index // 100
    return x * proj.SQ_U[0] + z * proj.SQ_V[0], x * proj.SQ_U[1] + z * proj.SQ_V[1]


def runs_of(squares):
    """Group the pieces of fringe roof squares into runs along a wall line.

    squares: {square index: [(edge, art)]}. A run is the pieces of one edge that
    sit in squares next to each other along the line: (edge, plane, start,
    members) with plane the tile line the wall stands on (v for edges 0 and 1,
    u for 2 and 3), start the first square along it, members [(square, art)].
    """
    lines = {}
    for index, pieces in squares.items():
        x, z = index % 100, index // 100
        for edge, art in pieces:
            plane = z + (edge == 1) if edge < 2 else x + (edge == 3)
            along = x if edge < 2 else z
            lines.setdefault((edge, plane), {})[along] = (index, art)
    runs = []
    for (edge, plane), members in sorted(lines.items()):
        start = previous = None
        for along in sorted(members):
            if previous is None or along != previous + 1:
                if start is not None:
                    runs.append((edge, plane, start, [members[a] for a in range(start, previous + 1)]))
                start = along
            previous = along
        runs.append((edge, plane, start, [members[a] for a in range(start, previous + 1)]))
    return runs


def unroll(edge, plane, start, members):
    """One run's strip: the pieces' art sheared along the wall line, as rgba.

    The strip is as wide as the run on the game's screen (48 px a tile along u,
    32 along v) and STRIP_ROWS high; its columns run left to right on the
    screen, its lowest painted row is the last row.
    """
    from fp import proj
    along_u = edge < 2
    count = len(members)
    step = -proj.SQ_U[0] if along_u else proj.SQ_V[0]
    width = int(round(count * step))
    slope = proj.U_SLOPE if along_u else proj.V_SLOPE
    ox, oy = proj.ORIGIN
    if along_u:
        # the line v = plane; its leftmost point is the end of the run
        line_x, line_y = ox + plane * proj.SQ_V[0], oy + plane * proj.SQ_V[1]
        left = line_x + (start + count) * proj.SQ_U[0]
    else:
        line_x, line_y = ox + plane * proj.SQ_U[0], oy + plane * proj.SQ_U[1]
        left = line_x + start * proj.SQ_V[0]
    stamped = []
    for index, art in members:
        tx, ty = _tile_screen(index)
        ys, xs = np.nonzero(art[..., 3] > 0)
        sx = ox + tx - 48 + xs  # the art's top vertex is at pixel 48
        sy = oy + ty + ys
        # the wall line's screen y under the pixel's centre; the row is the pixel's offset from it
        offset = (sy + 0.5) - (line_y + slope * (sx + 0.5 - line_x))
        stamped.append((np.round(sx - left).astype(int), np.floor(offset).astype(int), art[ys, xs]))
    low = max((rows[(cols >= 0) & (cols < width)].max() for cols, rows, _ in stamped
               if ((cols >= 0) & (cols < width)).any()), default=0)
    strip = np.zeros((STRIP_ROWS, width, 4), np.uint8)
    for cols, rows, rgba in stamped:
        r = rows - low + STRIP_ROWS - 1
        keep = (cols >= 0) & (cols < width) & (r >= 0) & (r < STRIP_ROWS)
        strip[r[keep], cols[keep]] = rgba[keep]
    return strip


BELT_ROWS = 16  # the band's lower part, from the wall's own art, in art pixels
EDGE_ROWS = 22  # the ragged edge on it: the strip's lowest rows (the concrete and the rebar's foot)
ODD_LUMA = 1.4  # a wall row whose picture is this much brighter or darker than the median row's is left out
SOLID_SHARE = 0.95  # of the wall picture's rows the belt takes: opaque, or it is no wall
BELT_LAP = 4    # the belt reaches this far up into the edge's rows, under its concrete


def belt_art(rows, walls, sprites, pages, hpx, run):
    """The band's belt of one run: the wall's own art continued upward, as rgba.

    A wall's front picture is the same for every storey that stands on it: the
    belt takes the picture's rows just under the wall's cap line (the plain
    concrete, never the top band above it) and goes up from the cap in the
    mirror of them, so it meets the wall without a seam, and one texel of it
    is one texel of the wall. Its columns are the strip's, each the wall's
    column at that place along the run. Returns (BELT_ROWS + BELT_LAP, width,
    4), its last row the lowest, or None when the wall has no picture to take.
    run: (edge, plane, start, members) as runs_of makes them.
    """
    from fp import proj
    edge, _, start, members = run
    axis = 0 if edge < 2 else 1
    count = len(members)
    width = int(round(count * (-proj.SQ_U[0] if edge < 2 else proj.SQ_V[0])))
    pieces = []
    for index in rows:
        row = walls[index]
        sa, sb = row[1 + axis], row[3 + axis]
        if sa == sb or not 0 <= int(row[0]) < len(sprites):
            continue
        page, x, y, w, h = (int(value) for value in sprites[int(row[0])][:5])
        top = int(round(min(max((row[6] - row[9]) * hpx, 0.0), h - 1.0)))
        # a picture with clear texels there (a fence, a window) is no wall to go on up
        block = pages[page][y + top:y + top + BELT_ROWS + BELT_LAP, x:x + w, 3]
        if block.size == 0 or (block == 255).mean() < SOLID_SHARE:
            continue
        pieces.append((min(sa, sb), max(sa, sb), sa, sb, pages[page], x, y + top, w, h - top,
                       float(pages[page][y + top:y + top + BELT_ROWS + BELT_LAP, x:x + w, :3].mean())))
    if not pieces or width <= 0:
        return None
    # a grate or a gate among plain wall rows is not what the wall goes on as: the brightness tells it
    median = float(np.median([piece[9] for piece in pieces]))
    pieces = [piece for piece in pieces if median / ODD_LUMA <= piece[9] <= median * ODD_LUMA]
    total = BELT_ROWS + BELT_LAP
    belt = np.zeros((total, width, 4), np.uint8)
    for column in range(width):
        # the strip's columns run left to right on the screen: against u, with v
        fraction = (column + 0.5) / width
        place = start + count - fraction * count if edge < 2 else start + fraction * count
        low, high, sa, sb, page, x, y, w, h, _ = min(
            pieces, key=lambda piece: max(piece[0] - place, place - piece[1], 0.0))
        # a place past the picture's ends (the run is longer than its wall's rows) repeats the picture
        at = x + min(int(((place - sa) / (sb - sa)) % 1.0 * w), w - 1)
        texels = page[y:y + h, at]
        solid = np.flatnonzero(texels[:, 3] == 255)
        if not len(solid):
            continue
        for j in range(total):
            # row j above the cap line takes the texel j under it, a clear one the next solid
            belt[total - 1 - j, column] = texels[solid[np.searchsorted(solid, min(j, h - 1)) % len(solid)]]
    return belt


def compose_band(strip, belt):
    """The strip of a run with its belt: BELT_ROWS + EDGE_ROWS rows of rgba.

    The ragged edge is the strip's lowest EDGE_ROWS rows, standing on the belt;
    the belt (opaque, from the wall's art) reaches BELT_LAP rows into the
    edge's, under its concrete, so no gap shows between them. Without a belt
    the strip's lowest rows stand alone.
    """
    total = BELT_ROWS + EDGE_ROWS
    edge = strip[-EDGE_ROWS:]
    if belt is None:
        return np.ascontiguousarray(strip[-total:])
    band = np.zeros((total, strip.shape[1], 4), np.uint8)
    band[EDGE_ROWS - BELT_LAP:] = belt
    painted = edge[..., 3] > 0
    band[:EDGE_ROWS][painted] = edge[painted]
    return band


WALL_REACH = 0.35  # a wall is under a roof edge when the edge lies this near its body, in tiles
END_REACH = 0.4    # a band's end meets the next wall within this reach beyond the run, in tiles


def wall_under(slabs, walls, edge, plane, start, count):
    """The wall a run of roof edge squares stands over, or None (see wall_group)."""
    group = wall_group(slabs, walls, edge, plane, start, count)
    return None if group is None else group[:4]


def wall_group(slabs, walls, edge, plane, start, count):
    """The wall a run of roof edge squares stands over, or None.

    slabs: wall_sidecar.wall_slabs(scene); walls: scene["walls"]. The roof edge
    line (v = plane for edges 0 and 1, u = plane for 2 and 3) lies in or just
    beside the body of the wall (its front face and the slab behind it); the
    wall lines along the run's line that do are grouped by their front plane,
    and the group covering most of the run is the run's wall. Returns (face,
    bottom, lo, hi, rows): the plane of the wall's outer face (the side away from
    the roof, in tile units), the wall top's height, the band's span along the
    line (the run's squares) and the wall's rows (indexes into walls).
    """
    axis = 0 if edge < 2 else 1
    outer = -1.0 if edge in (0, 2) else 1.0
    groups = {}
    for slab in slabs:
        if slab.axis != axis:
            continue
        low, high = slab.band()
        if not low - WALL_REACH <= plane <= high + WALL_REACH:
            continue
        weight = bottom = 0.0
        for index in slab.rows:
            row = walls[index]
            a, b = sorted((row[1], row[3]) if axis == 0 else (row[2], row[4]))
            share = min(b, start + count) - max(a, start)
            if share > 0:
                weight += share
                bottom += share * row[9]
        if weight <= 0.05:
            continue
        group = groups.setdefault(round(slab.plane, 2), [0.0, 0.0, low, high, []])
        group[0] += weight
        group[1] += bottom
        group[4].extend(slab.rows)
    if not groups:
        return None
    weight, bottom, low, high, rows = max(groups.values(), key=lambda g: (g[0], -abs(plane - (g[2] + g[3]) / 2)))
    return (high if outer > 0 else low), bottom / weight, float(start), float(start + count), rows


def write_fringe_art(out_png, roof_squares, art_of, wall_of, belt_of=None):
    """Write the unrolled strips of the roof's wall-top runs into one image, out_png.

    roof_squares: {square index (y * 100 + x): tile id} of the roof layer.
    art_of: tile id -> iso rgba. wall_of(edge, plane, start, count): the run's
    wall (wall_under) or None. belt_of(run, wall): the belt_art of the run, or None.
    Returns floor_sidecar's run entries (edge, plane,
    start, count, the strip's rect in out_png, the wall's face plane, top and the
    band's span). A run with no wall is left out. The image is not the
    tile atlas: the flat floor and roof squares sample that one, and its size
    must not change. With no wall-top tile no image is written (a stale one is
    removed) and the list is empty.
    """
    cache = {}

    def pieces_of(tile):
        if tile not in cache:
            art = art_of(tile)
            cache[tile] = edge_pieces(art) if is_fringe(art) else []
        return cache[tile]

    squares = {index: pieces_of(tile) for index, tile in sorted(roof_squares.items())}
    walls = []
    runs = []
    for run in runs_of({index: pieces for index, pieces in squares.items() if pieces}):
        wall = wall_of(run[0], run[1], run[2], len(run[3]))
        if wall is not None:
            runs.append(run)
            walls.append(wall)
    if not runs:
        out_png.unlink(missing_ok=True)
        return []
    strips = [compose_band(unroll(*run), belt_of(run, wall) if belt_of else None) for run, wall in zip(runs, walls)]
    width = max(strip.shape[1] for strip in strips) + 2
    height = sum(strip.shape[0] + 1 for strip in strips) + 1
    sheet = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    entries, y = [], 1
    for (edge, plane, start, members), strip, wall in zip(runs, strips, walls):
        sheet.paste(Image.fromarray(strip, "RGBA"), (1, y))
        entries.append((edge, plane, start, len(members), 1, y, strip.shape[1], strip.shape[0], *wall[:4]))
        y += strip.shape[0] + 1
    sheet.save(out_png, optimize=True)
    return entries
