"""Roof tiles that are a fringe: sparse, dark, ragged growth along a roof's edge.

The game draws roofs 96 pixels above the floor and paints a fringe tile's
blades as a picture that hangs down over the walls' tops. The exporter's
top-down tile cell flattens that picture onto the roof plane, where the blades
lie in streaks (task 129, find 6). The fringe tiles are told by their art, no
list: less than half of the tile's diamond is painted, the painted pixels
break up along each row (ragged) and are dark (growth, not a metal roof with
a hole or a solid trim strip). Their own art goes into roof_fringe.png and floor.bin
lists it; the renderer draws such a roof square as two
crossed upright sheets in place of the flat one, the same pixels in the
game's view.
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


def write_fringe_art(out_png, roof_cells, art_of):
    """Write the raw art of each fringe roof tile into its own image, out_png.

    roof_cells: {cell number: tile id} of the roof layer. art_of: tile id ->
    iso rgba. Returns floor_sidecar's fringe entries (cell number and the art's
    rect in out_png). The image is not the tile atlas: the flat floor and roof
    squares sample that one, and its size must not change. With no fringe tile
    no image is written (a stale one is removed) and the list is empty.
    """
    found = [(number, art_of(tile)) for number, tile in sorted(roof_cells.items())]
    found = [(number, rgba) for number, rgba in found if is_fringe(rgba)]
    if not found:
        out_png.unlink(missing_ok=True)
        return []
    tile_w = max(rgba.shape[1] for _, rgba in found) + 1
    tile_h = max(rgba.shape[0] for _, rgba in found) + 1
    per_row = max(1, min(len(found), 2048 // tile_w))
    rows = (len(found) + per_row - 1) // per_row
    sheet = Image.new("RGBA", (per_row * tile_w, rows * tile_h), (0, 0, 0, 0))
    entries = []
    for n, (number, rgba) in enumerate(found):
        x, y = (n % per_row) * tile_w, (n // per_row) * tile_h
        sheet.paste(Image.fromarray(rgba, "RGBA"), (x, y))
        entries.append((number, x, y, rgba.shape[1], rgba.shape[0]))
    sheet.save(out_png, optimize=True)
    return entries
