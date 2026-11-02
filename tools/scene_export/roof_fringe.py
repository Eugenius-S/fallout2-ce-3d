"""Roof tiles that are the broken tops of walls: ragged dark concrete along a roof's edge.

The game draws roofs 96 pixels above the floor. A roof's edge tile may carry
the broken top of the wall below it: a thin slab of concrete along one edge of
the tile with rebar and ragged concrete sticking up from it (task 129, find 6).
The exporter's top-down tile cell flattens that picture onto the roof plane,
where the rebar lies in streaks. These tiles are told by their art, no list:
less than half of the tile's diamond is painted, the painted pixels break up
along each row (ragged) and are dark (concrete, not a metal roof with a hole or
a solid trim strip).

Each such tile's art goes into roof_fringe.png as one piece per wall edge it
sits on (a corner tile has two). The renderer stands a piece up in the plane of
that edge, the plane of the wall below, so the wall rises above the roof under
the free camera (see roofs.cc).
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


def write_fringe_art(out_png, roof_cells, art_of):
    """Write the pieces of each fringe roof tile into their own image, out_png.

    roof_cells: {cell number: tile id} of the roof layer. art_of: tile id ->
    iso rgba. Returns floor_sidecar's fringe entries (cell number, edge and the
    piece's rect in out_png). The image is not the tile atlas: the flat floor and
    roof squares sample that one, and its size must not change. With no fringe
    tile no image is written (a stale one is removed) and the list is empty.
    """
    found = [(number, art_of(tile)) for number, tile in sorted(roof_cells.items())]
    found = [(number, rgba) for number, rgba in found if is_fringe(rgba)]
    pieces = [(number, edge, art) for number, rgba in found for edge, art in edge_pieces(rgba)]
    if not pieces:
        out_png.unlink(missing_ok=True)
        return []
    tile_w = max(art.shape[1] for _, _, art in pieces) + 1
    tile_h = max(art.shape[0] for _, _, art in pieces) + 1
    per_row = max(1, min(len(pieces), 2048 // tile_w))
    rows = (len(pieces) + per_row - 1) // per_row
    sheet = Image.new("RGBA", (per_row * tile_w, rows * tile_h), (0, 0, 0, 0))
    entries = []
    for n, (number, edge, art) in enumerate(pieces):
        x, y = (n % per_row) * tile_w, (n // per_row) * tile_h
        sheet.paste(Image.fromarray(art, "RGBA"), (x, y))
        entries.append((number, edge, x, y, art.shape[1], art.shape[0]))
    sheet.save(out_png, optimize=True)
    return entries
