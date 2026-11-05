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
Each run is matched to its wall (wall_under) by a general rule. The band's
face is the outer one of the wall's outer face and the roof's own boundary
line (a run with no wall stands on the boundary line), and where two bands
meet at a corner each ends on the other's face, so they close without a gap or
an overhang.

Every dark ragged texel on a roof's edge stands up (task 129, round 4): the
tiles are split per pixel (split_tile), told by the roof's footprint (the edges
that have no roof beyond), not by a whole-tile test. A mixed tile, shingle and a
ragged part in one diamond, keeps its shingle as a flat square in a cell of
its own in roof_fringe.png (the dark part cut out); the ragged part joins the
band of its edge.

One ragged edge per roof (task 129, round 6): a band built from genuine ragged
fringe tiles (a run with no slab-cut tile whose strip lies whole along the wall
line, genuine) is the roof's donor. Every other band of the roof (the slab-cut
sides, stepped or fragmentary runs) takes the donor's ragged edge, repeated
along its length (donate), and a side of the footprint that is not one genuine
run becomes one run over all its squares (whole_sides): the same height, the
ragged edge on top, no steps and no gaps. A roof with no donor takes the map's
best one. Bands that meet at a corner end on each other's faces, whatever edge
their art names, and share the lowest top of them (resolve_runs).
"""

import math

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


GREY_LUMA = 55.0    # concrete and rebar: a painted pixel this dark (mean of the colours) ...
GREY_TINT = 6.0     # ... and this grey (red and blue differ by less), shingle is brown
ALL_GREY = 0.85     # of a tile's painted pixels grey: the tile is all wall top
MIN_BLOB = 10       # dark pixels that hang together: fewer are specks in a roof's texture
WHOLE_COVER = 0.9   # of the diamond painted: a tile painted more is a whole roof square ...
RIM = 0.07          # tile art overhangs its square by this much, in tiles
FREE_OUTSIDE = 12   # ... unless more painted pixels than this stand off the diamond
LEFT_ROOF = 0.04    # of the diamond: a tile with less roof left is all wall top
EDGE_STEPS = ((0, -1), (0, 1), (-1, 0), (1, 0))  # the square beyond edge 0, 1, 2, 3: (dx, dz)


def boundary_edges(roof_squares):
    """{square index: [edges with no roof square beyond them]}: the roof's footprint boundary."""
    edges = {}
    for index in roof_squares:
        x, z = index % 100, index // 100
        found = [edge for edge, (dx, dz) in enumerate(EDGE_STEPS)
                 if not (0 <= x + dx < 100 and 0 <= z + dz < 100) or (z + dz) * 100 + x + dx not in roof_squares]
        edges[index] = found
    return edges


def _grid(rgba):
    """(painted, u, v, diamond) of a tile art: the ground (u, v) of every pixel, the diamond's mask."""
    from fp import proj
    painted = rgba[..., 3] > 0
    ys, xs = np.mgrid[0:painted.shape[0], 0:painted.shape[1]]
    u, v = proj.delta_to_ground(xs + 0.5 - 48, ys + 0.5)
    return painted, u, v, (u >= 0) & (u < 1) & (v >= 0) & (v < 1)


def _blobs(mask):
    """The connected groups (4 neighbours: a dithered checker is no blob) of a mask, as a list of (rows, cols)."""
    seen = np.zeros_like(mask)
    groups = []
    height, width = mask.shape
    for y0, x0 in zip(*np.nonzero(mask)):
        if seen[y0, x0]:
            continue
        stack, rows, cols = [(y0, x0)], [], []
        seen[y0, x0] = True
        while stack:
            y, x = stack.pop()
            rows.append(y)
            cols.append(x)
            for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                ny, nx = y + dy, x + dx
                if 0 <= ny < height and 0 <= nx < width and mask[ny, nx] and not seen[ny, nx]:
                    seen[ny, nx] = True
                    stack.append((ny, nx))
        groups.append((np.array(rows), np.array(cols)))
    return groups


def _grow(mask, painted, near):
    """The painted pixels with `near` or more of their 8 neighbours in the mask join it."""
    padded = np.pad(mask, 1).astype(int)
    count = sum(padded[1 + dy:padded.shape[0] - 1 + dy, 1 + dx:padded.shape[1] - 1 + dx]
                for dy in (-1, 0, 1) for dx in (-1, 0, 1) if dy or dx)
    return mask | (painted & (count >= near))


def _dilate(mask, reach):
    """The mask grown by `reach` pixels in every direction."""
    padded = np.pad(mask, reach)
    out = np.zeros_like(mask)
    for dy in range(-reach, reach + 1):
        for dx in range(-reach, reach + 1):
            out |= padded[reach + dy:reach + dy + mask.shape[0], reach + dx:reach + dx + mask.shape[1]]
    return out


SLAB_STRIP = 0.35   # a wall-top slab lies within this of an open edge, in tiles
SLAB_SHARE = 0.85   # of that strip the dark grey concrete covers this much: the strip is a slab


def slab_edges(rgba, open_edges):
    """The open edges of a roof square along which the tile's art is a wall-top slab: (edges, grey, dist).

    A tile can be painted whole and still carry the broken top of the wall: a
    strip of dark grey concrete, SLAB_STRIP wide, along the edge that has no
    roof beyond it (the square's footprint says which edges those are). A whole
    tile is no roof square then: the strip stands up as a band.
    """
    painted, u, v, diamond = _grid(rgba)
    if not painted.any() or not open_edges:
        return [], None, None
    rgb = rgba[..., :3].astype(float)
    grey = painted & (rgb.mean(axis=2) < GREY_LUMA) & (np.abs(rgb[..., 0] - rgb[..., 2]) < GREY_TINT)
    dist = np.stack([v, 1.0 - v, u, 1.0 - u])
    found = []
    for edge in open_edges:
        strip = diamond & (dist[edge] < SLAB_STRIP)
        if (grey & strip).sum() >= SLAB_SHARE * strip.sum():
            found.append(edge)
    return found, grey, dist


def all_grey_partial_tile(rgba):
    """Whether a partial roof tile is painted almost entirely as dark grey wall top."""
    painted, _, _, diamond = _grid(rgba)
    if not painted.any():
        return False
    cover = (painted & diamond).sum() / diamond.sum()
    if cover >= WHOLE_COVER:
        return False
    rgb = rgba[..., :3].astype(float)
    grey = painted & (rgb.mean(axis=2) < GREY_LUMA) & (np.abs(rgb[..., 0] - rgb[..., 2]) < GREY_TINT)
    return grey.sum() >= ALL_GREY * painted.sum()


def split_tile(rgba, open_edges=(), wall_supported=True):
    """A roof tile's art split into its wall-top part and its roof part.

    Returns (pieces, roof): pieces is [(edge, art)], the wall-top pixels split by
    the edge of the tile they stand on (edge_pieces; an empty list: a plain
    roof square), roof the art left as roof (rgba, the wall-top pixels cleared)
    or None when no roof is left. The wall-top pixels: those off the diamond
    (rebar and chunks above the wall), and the dark grey pixels (concrete,
    where shingle is brown) in blobs of MIN_BLOB or more, plus the painted pixels
    they enclose and the grey ones that touch the cut. A tile painted whole is a
    roof square, however dark: only what is off its diamond and the slab strips
    along its open edges are wall top, the rest stays flat roof. A tile painted
    less whose pixels are nearly all grey is all wall top.
    """
    painted, u, v, diamond = _grid(rgba)
    if not painted.any():
        return [], rgba
    cover = (painted & diamond).sum() / diamond.sum()
    # off the diamond: past the rim that tile art overhangs its square by (RIM)
    outside = painted & ~((u >= -RIM) & (u < 1 + RIM) & (v >= -RIM) & (v < 1 + RIM))
    rgb = rgba[..., :3].astype(float)
    # concrete and rebar are grey and dark, shingle is brown: red leads blue by more than a few levels
    grey = painted & (rgb.mean(axis=2) < GREY_LUMA) & (np.abs(rgb[..., 0] - rgb[..., 2]) < GREY_TINT)
    slabs, _, dist = slab_edges(rgba, open_edges)
    if cover < WHOLE_COVER and not wall_supported and grey.sum() >= ALL_GREY * painted.sum():
        return [], rgba
    if cover >= WHOLE_COVER and outside.sum() <= FREE_OUTSIDE and not slabs:
        return [], rgba
    whole = cover >= WHOLE_COVER
    if not whole and grey.sum() >= ALL_GREY * painted.sum():
        fringe = painted.copy()
    else:
        if whole:
            # a tile painted whole is a roof square, however dark: only what is off the diamond and the slab strips
            # along its open edges are wall top, the rest stays flat roof (a patch)
            fringe = outside.copy()
        else:
            keep = np.zeros_like(grey)
            for rows, cols in _blobs(grey):
                if len(rows) >= MIN_BLOB:
                    keep[rows, cols] = True
            fringe = outside | keep
        for edge in slabs:
            fringe |= grey & (dist[edge] < SLAB_STRIP + RIM)
        for _ in range(2):
            fringe = _grow(fringe, painted, 5)
        # the grey texels that touch the cut are its ragged rim: they go with it (a whole tile's cut is a straight
        # line at the slab strip: its grey beyond is roof texture)
        if not whole:
            fringe |= grey & _dilate(fringe, 2)
    if not fringe.any():
        return [], rgba
    art = rgba.copy()
    art[~fringe] = 0
    pieces = edge_pieces(art)
    roof = rgba.copy()
    roof[fringe] = 0
    left = ((roof[..., 3] > 0) & diamond).sum() / diamond.sum()
    return pieces, (roof if left >= LEFT_ROOF else None)


CELL_SIZE, CELL_MARGIN = 88, 6   # the tile atlas's cell (scene.py TILE_RES, TILE_MARGIN)


def unproject_cell(rgba):
    """A tile art as the atlas stores it: the square seen from above, (cell, cell, 4) rgba."""
    from fp import proj
    cell = CELL_SIZE + 2 * CELL_MARGIN
    t = (np.arange(cell) + 0.5 - CELL_MARGIN) / CELL_SIZE
    v, u = np.meshgrid(t, t, indexing="ij")
    src_x = np.floor(48 + u * proj.SQ_U[0] + v * proj.SQ_V[0]).astype(int)
    src_y = np.floor(u * proj.SQ_U[1] + v * proj.SQ_V[1]).astype(int)
    height, width = rgba.shape[:2]
    inside = (src_x >= 0) & (src_x < width) & (src_y >= 0) & (src_y < height)
    texture = np.zeros((cell, cell, 4), np.uint8)
    texture[inside] = rgba[src_y[inside], src_x[inside]]
    return texture


RAG_REACH = 5      # a roof texel this near a hole of its own square is the ragged rim of a cut: it goes (tidy_cell)
SLAB_HALF = 0.5    # a strip along an open edge that is this much dark grey is a wall top lying flat: cleared
THIN_ART = 3       # dark art thinner than 2 * THIN_ART + 1 texels is ragged (a finger, a rebar line, a stump)
SOLID_ART = 7      # painted texels among the 9 of its neighbourhood that make a texel solid art, not dither
DARK_ART = 60.0    # texels this dark (mean of the colours), any hue
MIN_LEFT = 150     # texels: a loose speck or needle left after the rim is cleared goes too


def tidy_cell(cell, open_edges):
    """A flat roof cell (rgba, CELL_SIZE + 2 * CELL_MARGIN square) with no dark ragged wall-top art left in it.

    The cells of roof_fringe.png are what the free camera draws flat, so a
    wall top left in one lies on the roof in flight (task 129). Whether or not a
    band stands on the edge, the cell keeps only plain roof: (1) along every
    open edge whose strip (SLAB_STRIP) is half dark grey or more the grey
    concrete is cleared; (2) the ragged rim, every texel within RAG_REACH of a
    hole inside the square, is cleared (a rim texel is the fingers and rebar
    stumps of a cut-out wall top; the square's own jagged rim and the art
    overhanging the square are not holes); (3) dark thin art left (a stump of a cut strip) goes, (4) loose specks go.
    A cell with no hole and no grey strip comes back unchanged.
    """
    cell = cell.copy()
    t = (np.arange(cell.shape[0]) + 0.5 - CELL_MARGIN) / CELL_SIZE
    v, u = np.meshgrid(t, t, indexing="ij")
    on = (u >= 0) & (u < 1) & (v >= 0) & (v < 1)
    dist = np.stack([v, 1.0 - v, u, 1.0 - u])
    rgb = cell[..., :3].astype(float)
    grey = (cell[..., 3] > 0) & (rgb.mean(axis=2) < GREY_LUMA) & (np.abs(rgb[..., 0] - rgb[..., 2]) < GREY_TINT)
    for edge in open_edges:
        strip = on & (dist[edge] < SLAB_STRIP)
        if (grey & strip).sum() >= SLAB_HALF * strip.sum():
            cell[grey & (dist[edge] < SLAB_STRIP + RIM)] = 0
    # a hole is a clear patch at least 3 x 3 texels; the clear texels of an alpha checkerboard (shading dither) are
    # diagonal neighbours only, vanish under this opening and so clear nothing around them
    clear = cell[..., 3] == 0
    clear = _dilate(~_dilate(~clear, 1), 1)
    hole = clear & (u > RIM) & (u < 1 - RIM) & (v > RIM) & (v < 1 - RIM)
    if hole.any():
        cell[_dilate(hole, RAG_REACH)] = 0
    # (3) what is left of dark art thinner than 2 * THIN_ART + 1 texels (a stump of a cut strip, a rebar line) goes;
    # a checkerboard dither (few painted neighbours) is shading, it stays
    painted = cell[..., 3] > 0
    if painted.any():
        grown = np.pad(painted, THIN_ART, mode="edge")
        opened = _dilate(~_dilate(~grown, THIN_ART), THIN_ART)[THIN_ART:-THIN_ART, THIN_ART:-THIN_ART]
        padded = np.pad(painted, 1).astype(int)
        solid = sum(padded[1 + dy:padded.shape[0] - 1 + dy, 1 + dx:padded.shape[1] - 1 + dx]
                    for dy in (-1, 0, 1) for dx in (-1, 0, 1)) >= SOLID_ART
        thin = painted & ~opened & solid & (cell[..., :3].astype(float).mean(axis=2) < DARK_ART)
        cell[thin] = 0
    left = cell[..., 3] > 0
    if left.any():
        # a clear texel with painted texels on all four sides is a dither gap: it joins the blob it sits in, so a
        # checkerboard counts as one piece of shading, not as a thousand loose specks
        around = np.pad(left, 1)
        gap = ~left & around[:-2, 1:-1] & around[2:, 1:-1] & around[1:-1, :-2] & around[1:-1, 2:]
        for rows, cols in _blobs(left | gap):
            size = int(left[rows, cols].sum())
            if size < MIN_LEFT:
                cell[rows, cols] = 0
    return cell


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


WALL_TOUCH = 0.03  # wall slabs whose bands lie this near each other are one wall, in tiles
WALL_SAME_TOP = 0.1  # and top out within this of each other (GL units)


def wall_body(slabs, walls, edge, plane, start, count):
    """The wall a run of roof edge squares stands over, or None.

    slabs: wall_sidecar.wall_slabs(scene); walls: scene["walls"]. The roof edge
    line (v = plane for edges 0 and 1, u = plane for 2 and 3) lies in or just
    beside the body of the wall (its front face and the slab behind it); the
    wall lines along the run's line that do are grouped by their front plane,
    and the group covering most of the run is the run's wall. Returns (low,
    high, bottom, first, last, rows): the planes of the wall's two faces (tile
    units, low < high), the wall top's height, where the wall's rows begin and
    end along the line and its rows (indexes into walls).
    """
    axis = 0 if edge < 2 else 1
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
        group = groups.setdefault(round(slab.plane, 2), [0.0, 0.0, low, high, [], math.inf, -math.inf])
        group[0] += weight
        group[1] += bottom
        group[2], group[3] = min(group[2], low), max(group[3], high)
        group[4].extend(slab.rows)
        group[5] = min(group[5], slab.lo)
        group[6] = max(group[6], slab.hi)
    if not groups:
        return None
    chosen = max(groups.values(), key=lambda g: (g[0], -abs(plane - (g[2] + g[3]) / 2)))
    weight, bottom, low, high, rows, first, last = chosen
    # slabs of one wall at other front planes (a cap, a thicker stretch) that touch its band and top out at
    # the same height are part of it: the wall's faces are those of the whole
    pending = [g for g in groups.values() if g is not chosen]
    grown = True
    while grown:
        grown = False
        for g in list(pending):
            if g[2] <= high + WALL_TOUCH and g[3] >= low - WALL_TOUCH and abs(g[1] / g[0] - bottom / weight) <= WALL_SAME_TOP:
                weight, bottom = weight + g[0], bottom + g[1]
                low, high = min(low, g[2]), max(high, g[3])
                rows = rows + g[4]
                first, last = min(first, g[5]), max(last, g[6])
                pending.remove(g)
                grown = True
    return low, high, bottom / weight, first, last, rows


def wall_group(slabs, walls, edge, plane, start, count, outer=None):
    """wall_body with the face on the outer side: (face, bottom, lo, hi, rows) or None.

    outer: -1 or 1, the side of the wall away from the roof (by default the edge's
    own side: -1 for edges 0 and 2, 1 for 1 and 3). lo, hi: the run's squares.
    """
    body = wall_body(slabs, walls, edge, plane, start, count)
    if body is None:
        return None
    if outer is None:
        outer = -1.0 if edge in (0, 2) else 1.0
    return (body[1] if outer > 0 else body[0]), body[2], float(start), float(start + count), body[5]


OUTWARD_DEPTH = 6  # squares beyond a wall that tell the roof is on its far side


def outward(roof_squares, edge, low, high, start, count, depth=OUTWARD_DEPTH):
    """Which side of a wall is the building's outside: -1 (lower u or v) or 1.

    The roof's footprint thins out toward the outside: the side with fewer roof
    squares in the OUTWARD_DEPTH squares beyond the wall's face, along the run.
    A tie goes to the side the roof edge's art names (edge 0, 2: lower).
    """
    along = range(start, start + count)
    rows_below = [int(math.floor(low - 1e-6)) - 1 - d for d in range(depth)]
    rows_above = [int(math.ceil(high + 1e-6)) + d for d in range(depth)]
    cell = (lambda a, r: (a, r)) if edge < 2 else (lambda a, r: (r, a))
    below = [cell(a, r) for r in rows_below for a in along]
    above = [cell(a, r) for r in rows_above for a in along]
    count_of = lambda cells: sum(1 for x, z in cells if 0 <= x < 100 and 0 <= z < 100 and z * 100 + x in roof_squares)
    lower, upper = count_of(below), count_of(above)
    if lower != upper:
        return -1.0 if lower < upper else 1.0
    return -1.0 if edge in (0, 2) else 1.0


MIN_SPAN = 0.05  # a wall that covers less of a run than this is not under it, in tiles
CORNER_REACH = 0.7  # two bands meet at a corner when it lies this near both their ends, in tiles


def resolve_runs(runs, bodies, outers, default_top):
    """Where each run's band stands: [(face, bottom, lo, hi)] in tile units, one per run.

    bodies: wall_body for each run, or None; outers: the side of each run's
    wall that is the building's outside (see outward). The face is the wall's
    outer face (a run with no wall stands on the tile line its art names); the
    band's span the run's squares, cut to where the wall's rows end. Where the
    ends of two bands lie near the corner their lines make, each ends on the
    other's face: no gap, no overhang.
    """
    out = []
    for (edge, plane, start, members), body, outer in zip(runs, bodies, outers):
        lo, hi = float(start), float(start + len(members))
        if body is None:
            out.append([float(plane), default_top, lo, hi])
            continue
        span = [max(lo, body[3]), min(hi, body[4])]
        if span[1] - span[0] < MIN_SPAN:
            # the wall's rows end before this run's squares: no wall under it
            out.append([float(plane), default_top, lo, hi])
            continue
        out.append([body[1] if outer > 0 else body[0], body[2], *span])
    original = [list(place) for place in out]
    parent = list(range(len(runs)))

    def root(n):
        while parent[n] != n:
            n = parent[n]
        return n

    for n, (edge, plane, start, members) in enumerate(runs):
        for side in (2, 3):
            # the nearest perpendicular band across this end: this band's line is its face across the
            # other's span, the other's face across this one's (whichever side of the corner it lies on)
            best = None
            for m, (other_edge, other_plane, other_start, other_members) in enumerate(runs):
                if n == m or (other_edge < 2) == (edge < 2):
                    continue
                corner_along, corner_cross = original[m][0], original[n][0]
                gap = abs(original[n][side] - corner_along)
                if gap <= CORNER_REACH and original[m][2] - CORNER_REACH <= corner_cross <= original[m][3] + CORNER_REACH                         and (best is None or gap < best[0]):
                    best = (gap, corner_along, m)
            # a snap that would empty or reverse the span is not made
            if best is not None and (out[n][3] - best[1] if side == 2 else best[1] - out[n][2]) >= MIN_SPAN:
                out[n][side] = best[1]
                parent[root(n)] = root(best[2])
    # the bands that meet at corners are one parapet: one top line, the lowest wall top of them
    lowest = {}
    for n in range(len(runs)):
        lowest[root(n)] = min(lowest.get(root(n), math.inf), out[n][1])
    for n in range(len(runs)):
        out[n][1] = lowest[root(n)]
    return out


def genuine(run, strip, slab_cut):
    """Is this run built from genuine ragged fringe art: no slab-cut tile in it, and a strip that lies whole along the wall line."""
    return not any(index in slab_cut for index, _ in run[3]) and foot_cover(strip) >= FOOT_COVER


def roof_sides(roof_squares, open_of):
    """The sides of the roof footprint: [(edge, plane, start, [square index])], contiguous squares sharing an open edge.

    edge, plane, start as in runs_of, but by the footprint (the square's open
    edge: no roof beyond it), the way the slab-cut squares name theirs.
    """
    lines = {}
    for index in roof_squares:
        x, z = index % 100, index // 100
        for edge in open_of[index]:
            plane = z + (edge == 1) if edge < 2 else x + (edge == 3)
            lines.setdefault((edge, plane), {})[x if edge < 2 else z] = index
    sides = []
    for (edge, plane), members in sorted(lines.items()):
        group = []
        for along in sorted(members):
            if group and along != group[-1][0] + 1:
                sides.append((edge, plane, group[0][0], [index for _, index in group]))
                group = []
            group.append((along, members[along]))
        sides.append((edge, plane, group[0][0], [index for _, index in group]))
    return sides


def whole_sides(runs, strips, good, sides):
    """Runs and strips with every side of the footprint that is not one genuine run made one run.

    A side whose runs (those of its axis standing on its squares) are one
    genuine run over all its squares is left as it is. Any other side with a
    wall top on it (a slab-cut, fragmentary or stepped run) becomes one run over
    all its squares, strip None (it takes the donor's, see donate): continuous
    along the side, no steps, no gaps.
    """
    drop, added = set(), []
    # each run belongs to one physical side: of the sides of its axis that cover its squares, the one on its
    # own line (a roof one square deep has two sides on the same squares)
    owner = {}
    for n, run in enumerate(runs):
        near = [k for k, (edge, plane, start, squares) in enumerate(sides)
                if (edge < 2) == (run[0] < 2) and all(index in set(squares) for index, _ in run[3])]
        if near:
            owner[n] = min(near, key=lambda k: (abs(run[1] - sides[k][1]), run[0] != sides[k][0]))
    for k, (edge, plane, start, squares) in enumerate(sides):
        inside = [n for n in range(len(runs)) if n not in drop and owner.get(n) == k]
        if not inside:
            continue
        if len(inside) == 1 and good[inside[0]] and len(runs[inside[0]][3]) == len(squares):
            continue
        drop.update(inside)
        # the merged run keeps the art the dropped runs had, to fall back on when there is no donor
        had = {index: art for n in inside for index, art in runs[n][3]}
        added.append(((edge, plane, start, [(index, had.get(index)) for index in squares]), None, False))
    keep = [n for n in range(len(runs)) if n not in drop]
    return ([runs[n] for n in keep] + [item[0] for item in added],
            [strips[n] for n in keep] + [None] * len(added),
            [good[n] for n in keep] + [False] * len(added))


BLANK = ('blank',)  # the key of the transparent cell of a roof square that is all wall top and has no band


def cut_to_walls(runs, strips, good, wall_of):
    """Runs and strips with every run cut into the stretches that have a wall under them.

    wall_of(edge, plane, start, count): the run's wall (wall_body) or None. A
    band stands only where a wall stands under it: the squares of a run are
    tested one by one, the contiguous squares with a wall are one run each (a
    strip None, so the donor's ragged edge, unless the whole run has a wall), and
    the squares with none (open ground, a hole in the roof, a broken wall) get no
    band: a side is never bridged over a gap.
    """
    out_runs, out_strips, out_good = [], [], []
    for (edge, plane, start, members), strip, ok in zip(runs, strips, good):
        has = [wall_of(edge, plane, start + n, 1) is not None for n in range(len(members))]
        if all(has):
            out_runs.append((edge, plane, start, members))
            out_strips.append(strip)
            out_good.append(ok)
            continue
        n = 0
        while n < len(members):
            if not has[n]:
                n += 1
                continue
            m = n
            while m < len(members) and has[m]:
                m += 1
            out_runs.append((edge, plane, start + n, [(index, art) for index, art in members[n:m]]))
            out_strips.append(None)
            out_good.append(False)
            n = m
    return out_runs, out_strips, out_good


def donate(runs, strips, good, pool, comp, diagnostics=None):
    """Give every band on one roof the same genuine ragged edge.

    pool: [(run, strip, component)] of the map's genuine runs. A roof (a
    connected footprint) takes the longest genuine run of its own as donor, else
    the longest genuine run of the map. All of that roof's bands, including other
    genuine runs, take the donor's ragged edge, repeated along their length. This
    keeps one roof edge from having a different height or thickness than another.
    """
    def profile(item):
        return (item[1][-EDGE_ROWS:, :, 3] > 0).mean(axis=1)

    def donor(items):
        if not items:
            return None
        profiles = [profile(item) for item in items]
        return min(items, key=lambda item: (
            sum(float(np.abs(profile(item) - other).mean()) for other in profiles),
            -foot_cover(item[1]),
            -len(item[0][3]),
        ))

    map_donor = donor(pool)
    donors = {}
    out = []
    for run, strip, ok in zip(runs, strips, good):
        component = comp[run[3][0][0]] if run[3] else None
        if component not in donors:
            donors[component] = donor([item for item in pool if item[2] == component]) or map_donor
        pick = donors[component]
        if pick is None:
            out.append(strip if strip is not None else own_strip(run))
            continue
        out.append(lay_donor((pick[1], len(pick[0][3]), pick[0][0]), run[0], len(run[3])))
        if diagnostics is not None:
            diagnostics.append((run[:3], pick[0][:3]))
    return out


def own_strip(run):
    """The strip a run unrolls from the art its own squares kept (a square without art stays clear)."""
    shapes = [art.shape for _, art in run[3] if art is not None]
    if not shapes:
        return np.zeros((STRIP_ROWS, int(round(len(run[3]) * (-proj_u() if run[0] < 2 else proj_v()))), 4), np.uint8)
    members = [(index, art if art is not None else np.zeros(shapes[0], np.uint8)) for index, art in run[3]]
    return unroll(run[0], run[1], run[2], members)


def proj_u():
    from fp import proj
    return proj.SQ_U[0]


def proj_v():
    from fp import proj
    return proj.SQ_V[0]


FOOT_ROWS = 3       # a genuine strip's lowest rows: the concrete lies along the wall line ...
FOOT_COVER = 0.9    # ... so this share of its columns is painted in them: no gaps, no steps


def foot_cover(strip):
    """The share of a strip's columns painted in its lowest FOOT_ROWS rows (see unroll)."""
    return float((strip[-FOOT_ROWS:, :, 3] > 0).any(axis=0).mean())


def components_of(roof_squares):
    """{square index: component number}: the 4-connected pieces of a roof footprint."""
    comp = {}
    for first in sorted(roof_squares):
        if first in comp:
            continue
        comp[first] = first
        stack = [first]
        while stack:
            index = stack.pop()
            x, z = index % 100, index // 100
            for dx, dz in EDGE_STEPS:
                other = (z + dz) * 100 + x + dx
                if 0 <= x + dx < 100 and other in roof_squares and other not in comp:
                    comp[other] = first
                    stack.append(other)
    return comp


def lay_donor(donor, edge, count):
    """A strip for a run of `count` squares along `edge`'s axis made of the donor's ragged edge, repeated.

    donor: (strip, count, edge) of a genuine run. The donor's columns are
    resampled to the target axis's width per tile (48 px along u, 32 along v)
    and repeated along the run; its rows (ragged edge on top, the concrete along
    the bottom) are kept, so the band keeps the donor's height and orientation.
    """
    from fp import proj
    strip, donor_count, donor_edge_id = donor
    edge_rows = strip[-EDGE_ROWS:]
    per_tile = lambda e: -proj.SQ_U[0] if e < 2 else proj.SQ_V[0]
    width = int(round(count * per_tile(edge)))
    unit = int(round(donor_count * per_tile(donor_edge_id)))
    unit_width = int(round(donor_count * per_tile(edge)))
    pick = np.minimum((np.arange(unit_width) + 0.5) * unit / unit_width, unit - 1).astype(int)
    unit_art = edge_rows[:, pick]
    out = np.zeros((STRIP_ROWS, width, 4), np.uint8)
    out[-EDGE_ROWS:] = np.tile(unit_art, (1, width // unit_width + 1, 1))[:, :width]
    return out


DONOR_COVER = 0.9   # of the square: a donor tile's cell is painted this much (a plain roof, not a trim strip or a ragged edge)


def _cover(cell):
    """The share of the square (u, v in 0..1) that a flat cell paints."""
    t = (np.arange(cell.shape[0]) + 0.5 - CELL_MARGIN) / CELL_SIZE
    v, u = np.meshgrid(t, t, indexing="ij")
    on = (u >= 0) & (u < 1) & (v >= 0) & (v < 1)
    return float(((cell[..., 3] > 0) & on).sum() / on.sum())


def plain_art(rgba):
    """Whether a tile art is plain roof: its tidied cell painted DONOR_COVER or more (not a trim strip or a ragged edge)."""
    return _cover(tidy_cell(unproject_cell(rgba), ())) >= DONOR_COVER


def donor_tiles(roof_squares, open_of, banded, comp, art_of):
    """{component: tile id}: the plain roof tile each roof draws where a banded square has no roof art of its own.

    The most common roof tile among the non-boundary squares of the component,
    else the most common among its squares no band crosses. A tile whose tidied
    cell is not plain roof (painted less than DONOR_COVER) is passed over. A
    component with no plain tile of its own (a ring of wall tops round a courtyard,
    a ruin open to the sky) has none: it is no roof, only its bands stand.
    """
    inner, loose = {}, {}
    for index, tile in roof_squares.items():
        if not open_of[index]:
            inner.setdefault(comp[index], {}).setdefault(tile, 0)
            inner[comp[index]][tile] += 1
        if index not in banded:
            loose.setdefault(comp[index], {}).setdefault(tile, 0)
            loose[comp[index]][tile] += 1
    plain = {}

    def is_plain(tile):
        if tile not in plain:
            plain[tile] = plain_art(art_of(tile))
        return plain[tile]

    out = {}
    for number in set(comp.values()):
        for counts in (inner.get(number, {}), loose.get(number, {})):
            found = [tile for tile, _ in sorted(counts.items(), key=lambda item: (-item[1], item[0])) if is_plain(tile)]
            if found:
                out[number] = found[0]
                break
    return out


def band_clips(runs, places, outers):
    """{square index: ((axis, face, side), ...)}: the planes of the bands that cross each square.

    axis 0: the plane v = face (a band along u), 1: u = face (a band along v),
    face in the square's own tile units; side 1 when the building's outside is
    the higher side of the plane, -1 when the lower. The roof lies on the other
    side (the inner one).
    """
    clips = {}
    for (edge, plane, start, members), place, outer in zip(runs, places, outers):
        axis = 0 if edge < 2 else 1
        side = (outer if outer is not None else (-1.0 if edge in (0, 2) else 1.0)) > 0
        for index, _ in members:
            base = index // 100 if axis == 0 else index % 100
            clips.setdefault(index, set()).add((axis, round(place[0] - base, 3), 1 if side else -1))
    return {index: tuple(sorted(found)) for index, found in clips.items()}


RING = "ring"  # the donor of a banded square with no roof behind it: none, its cell is only cut at the band


def inner_edge(clip):
    """The edge of its square (EDGE_STEPS) a band_clips plane faces the roof across: the one away from the outside."""
    axis, _, side = clip
    return (0 if side > 0 else 1) if axis == 0 else (2 if side > 0 else 3)


def cut_cell(cell, clips, donor):
    """A flat roof cell cut at the planes of the bands that cross its square and filled with plain roof up to them.

    clips: band_clips of the square. Nothing is left on the outer side of a
    plane; every clear texel on the inner side (a hole where the wall top was cut
    out, a square with no roof art of its own) takes the donor cell's texel (donor None: none does). A
    clear texel with painted texels on all four sides is shading dither and stays.
    """
    t = (np.arange(cell.shape[0]) + 0.5 - CELL_MARGIN) / CELL_SIZE
    v, u = np.meshgrid(t, t, indexing="ij")
    inner = np.ones(cell.shape[:2], bool)
    for axis, face, side in clips:
        coord = v if axis == 0 else u
        inner &= (coord < face) if side > 0 else (coord >= face)
    out = cell.copy()
    out[~inner] = 0
    painted = out[..., 3] > 0
    around = np.pad(painted, 1)
    dither = ~painted & around[:-2, 1:-1] & around[2:, 1:-1] & around[1:-1, :-2] & around[1:-1, 2:]
    if donor is not None:
        fill = inner & ~painted & ~dither
        out[fill] = donor[fill]
    return out


def write_fringe_art(out_png, roof_squares, art_of, wall_of, belt_of=None, default_top=1.88, diagnostics=None):
    """Write the unrolled strips of the roof's wall tops, and the flat squares cut around them, into one image, out_png.

    roof_squares: {square index (y * 100 + x): tile id} of the roof layer.
    art_of: tile id -> iso rgba. wall_of(edge, plane, start, count): the run's
    wall (wall_body) or None. belt_of(run, wall): the belt_art of the run, or None.
    Returns (runs, patches): floor_sidecar's run entries (edge, plane, start,
    count, the strip's rect in out_png, the wall's face plane, top and the band's
    span; see resolve_runs) and its patch entries (square index, x, y: the cell
    in out_png a mixed square's roof is drawn from under the free camera, its
    wall-top part cut out). The image is not the tile atlas: the flat floor and
    roof squares sample that one, and its size must not change. With no wall top
    and no flat cell to tidy no image is written (a stale one is removed) and both lists are empty; when every
    run is cut away (no wall stands under it) the image still holds the tidied cells and the patches name them.
    """
    cache = {}

    def split(tile, open_edges, wall_supported):
        key = (tile, open_edges, wall_supported)
        if key not in cache:
            cache[key] = split_tile(art_of(tile), open_edges, wall_supported)
        return cache[key]

    open_of = boundary_edges(roof_squares)
    squares, patched, slab_cut = {}, {}, set()
    unsupported_partial = set()
    for index, tile in sorted(roof_squares.items()):
        open_edges = tuple(open_of[index])
        wall_supported = True
        art = art_of(tile)
        if all_grey_partial_tile(art):
            x, z = index % 100, index // 100
            wall_supported = False
            for edge in open_edges:
                plane = z + (edge == 1) if edge < 2 else x + (edge == 3)
                start = x if edge < 2 else z
                if wall_of(edge, plane, start, 1) is not None:
                    wall_supported = True
                    break
            if not wall_supported:
                unsupported_partial.add(index)
        pieces, roof = split(tile, open_edges, wall_supported)
        if pieces:
            squares[index] = pieces
            patched[index] = ((tile, tuple(open_of[index])), roof)
            if slab_edges(art_of(tile), tuple(open_of[index]))[0]:
                slab_cut.add(index)
    runs = runs_of(squares)
    raw = [unroll(*run) for run in runs]
    good = [genuine(run, strip, slab_cut) for run, strip in zip(runs, raw)]
    comp = components_of(roof_squares)
    pool = [(run, strip, comp[run[3][0][0]]) for run, strip, ok in zip(runs, raw, good) if ok]
    runs, raw, good = whole_sides(runs, raw, good, roof_sides(roof_squares, open_of))
    runs, raw, good = cut_to_walls(runs, raw, good, wall_of)
    # a square that is all wall top and has no band left (no wall under it) is not drawn at all
    banded = {index for run in runs for index, _ in run[3]}
    for index, ((tile, open_edges), roof) in list(patched.items()):
        if roof is None and index not in banded:
            patched[index] = (BLANK, np.zeros_like(art_of(tile)))
    strips = donate(runs, raw, good, pool, comp, diagnostics)
    bodies = [wall_of(run[0], run[1], run[2], len(run[3])) for run in runs]
    outers = [None if body is None else outward(roof_squares, run[0], body[0], body[1], run[2], len(run[3]))
              for run, body in zip(runs, bodies)]
    places = resolve_runs(runs, bodies, outers, default_top)
    for place in places:
        if not place[3] > place[2]:
            raise ValueError(f"a roof band's span is empty or reversed: {place}")
    strips = [compose_band(strip, belt_of(run, body) if belt_of and body is not None else None)
              for run, body, strip in zip(runs, bodies, strips)]
    # a square the bands cross keeps its flat roof under the free camera only as a patch: a plain one gets its art
    for index in banded:
        if index not in patched:
            patched[index] = ((roof_squares[index], tuple(open_of[index])), art_of(roof_squares[index]))
    # and it is roofed right up to the band: its cell is cut at the band's plane and filled with the plain roof of
    # its component up to it (a square with no donor keeps its cell, or none). Only a square with plain roof behind it: one
    # whose inner side opens on a courtyard or on more wall tops is a ring round an open ruin, not a roof (cut, not filled)
    clips = band_clips(runs, places, outers)
    donors = donor_tiles(roof_squares, open_of, banded, comp, art_of)
    def behind(index):
        return [(index // 100 + EDGE_STEPS[edge][1]) * 100 + index % 100 + EDGE_STEPS[edge][0]
                for edge in {inner_edge(clip) for clip in clips[index]}]

    plain_tiles = {}

    def plain(index):
        if index not in roof_squares or index in banded:
            return False
        tile = roof_squares[index]
        if tile not in plain_tiles:
            plain_tiles[tile] = plain_art(art_of(tile))
        return plain_tiles[tile]

    # a square is roofed when what lies behind each of its bands is plain roof or a roofed square (a corner): grown out
    # from the plain roof, so two strips of wall tops facing each other never vouch for each other
    candidates = {index for index in banded if not any(inner_edge(clip) in open_of[index] for clip in clips[index])}
    roofed = set()
    while True:
        grown = {index for index in candidates - roofed if all(back in roofed or plain(back) for back in behind(index))}
        if not grown:
            break
        roofed |= grown
    for index in sorted(banded):
        donor = donors.get(comp[index])
        if donor is None:
            continue
        if index not in roofed:
            donor = RING   # cut at the band, not filled
        key, roof = patched[index]
        if key == BLANK:
            key = (roof_squares[index], tuple(open_of[index]))
        patched[index] = ((*key, clips[index], donor), roof if roof is not None else np.zeros_like(art_of(roof_squares[index])))
    # a boundary square no band crosses can carry a ragged dark wall top the classifier did not see (black fingers
    # along its open edge): it too is drawn from a tidied cell, when tidying changes it
    tidied = {}
    for index, tile in sorted(roof_squares.items()):
        if index in patched or not open_of[index] or index in unsupported_partial:
            continue
        key = (tile, tuple(open_of[index]))
        if key not in tidied:
            flat = unproject_cell(art_of(tile))
            tidied[key] = bool((tidy_cell(flat, key[1]) != flat).any())
        if tidied[key]:
            patched[index] = (key, art_of(tile))
    # every flat cell is tidied: no dark ragged wall top lies flat, whether or not a band stands on its edge
    cells = {}
    for index, (tile, roof) in patched.items():
        if roof is not None and tile not in cells:
            if tile == BLANK:
                cells[tile] = tidy_cell(unproject_cell(roof), ())
            else:
                cells[tile] = tidy_cell(unproject_cell(roof), tile[1])
                if len(tile) == 4:
                    cells[tile] = cut_cell(cells[tile], tile[2],
                                           None if tile[3] == RING else tidy_cell(unproject_cell(art_of(tile[3])), ()))
    cell = CELL_SIZE + 2 * CELL_MARGIN
    if not strips and not cells:
        out_png.unlink(missing_ok=True)
        return [], []
    widest = max((strip.shape[1] for strip in strips), default=0)
    per_row = max(1, widest // (cell + 1))
    width = max(widest + 2, per_row * (cell + 1) + 1)
    cell_rows = (len(cells) + per_row - 1) // per_row
    height = sum(strip.shape[0] + 1 for strip in strips) + 1 + cell_rows * (cell + 1)
    sheet = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    entries, y = [], 1
    for (edge, plane, start, members), strip, place in zip(runs, strips, places):
        sheet.paste(Image.fromarray(strip, "RGBA"), (1, y))
        entries.append((edge, plane, start, len(members), 1, y, strip.shape[1], strip.shape[0], *place))
        y += strip.shape[0] + 1
    where = {}
    for n, (tile, texture) in enumerate(cells.items()):
        where[tile] = (1 + (n % per_row) * (cell + 1), y + (n // per_row) * (cell + 1))
        sheet.paste(Image.fromarray(texture, "RGBA"), where[tile])
    patches = [(index, *where[tile]) for index, (tile, roof) in sorted(patched.items()) if roof is not None]
    sheet.save(out_png, optimize=True)
    return entries, patches
