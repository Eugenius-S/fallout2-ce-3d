"""Scenery with depth: a box or a drum in place of a sprite card.

A sprite is one picture of an object, taken by the game's camera. For first
person the object gets a simple solid instead - a box along the floor axes, or
an upright drum - and the viewer textures it by projection from the game's
camera: a point of the solid shows the pixel it lands on in the sprite
(viewer/render.js). The game's camera therefore still sees the sprite,
every other eye sees a thing with depth. This module chooses the solid: it
reads its size out of the sprite's outline.

On the game's screen a step along u goes 48 px left and 12 down, a step along
v 32 right and 24 down, a step up HPX px up (proj.py). So

    a box       projects to a hexagon: its top face is a parallelogram with
                edges of slope -1/4 (along u) and +3/4 (along v), and two more
                parallelograms hang from the top's two near edges;
    a drum      to a capsule: an ellipse (K times as high as wide) for the top
                and half of one for the foot.

Every fit is described by the outline of its top face at height z1, `far(x)`
and `near(x)` px per screen column, and the height z1 - z0 of what hangs from
it. Two ways to find them, tried in this order (`fit`):

    from the floor   the sprite's lower outline gives the foot: for a box the
                     two supporting lines of slope +3/4 and -1/4 from below
                     (they meet in its near corner), for a drum the ellipse
                     that the lower outline rests on. The height is the one
                     that keeps the top face filled: raising the top takes in
                     more of the sprite but moves the top face off it
                     (`_raise`). Good for everything that stands on the floor
                     with its whole foot or on legs at its corners: beds,
                     crates, lockers, shelves, tables, barrels - whatever may
                     stand on top of them or rise behind (a headboard).
    from the top     the upper outline gives the top face (supporting lines
                     from above), the solid hangs from it as far down as the
                     sprite stays as wide, and the rest is a stem that reaches
                     the floor under the middle: round tables, lamp posts.

Neither is accepted unless the sprite really has that outline and a filled top
face (trees, people-like things and piles do not): those stay cards. A long box
with filled sides is then cut across into slices, and every slice gets its own
height (`_slices`): a car is a low box with a higher one for its cabin, a
conveyor a low one with the machines at its end standing taller.

What a solid does not cover is its `rest`: pixels above it (a headboard,
flames, what stands on a table) and the stem under it. Above a box they go
onto the planes of its two far sides, which rise as far as needed (`boxes`):
that is where a headboard is and a back board would be. A drum's rest and all
stems become upright cards on the solid's axis (scene.py).

`groups` and `compose` at the end put back together what the artists cut into
several map objects, so that it is fitted as one thing.
"""
import numpy as np

from . import proj, walls

U, V = proj.U_SLOPE, proj.V_SLOPE               # screen slopes of lines along u and along v
PX_U, PX_V = -proj.SQ_U[0], proj.SQ_V[0]        # px to the left per unit of u, to the right per unit of v
K = np.hypot(proj.SQ_U[1], proj.SQ_V[1]) / np.hypot(proj.SQ_U[0], proj.SQ_V[0])     # height / width of a floor circle
EMPTY = 4.0             # what a transparent pixel of the top face costs, in opaque pixels gained
TOP_FILL = 0.8          # share of the top face that must be opaque
OPEN_TOP_FILL, WHOLE = 0.6, 0.9     # ... or this, if the solid holds this share of the sprite and stands on legs
STEM_TOP_FILL = 0.9     # ... when the outline was read from the top (nothing else vouches for it)
OUTLINE_PX = 1.6        # mean distance (px) of the sprite's outline from the model's, at most
LID_OUTLINE_PX = 3.5    # ... for a lid drum: its sides are the fly camera's alone, the base may
                        # tuck under the belly a little more than a plain drum's may
CLIP_PX = 6.0           # a column counts for no more than this in that mean
CORNER_PX = 10          # a box's three near floor corners have sprite within this distance (legs, at least)
LEG_STRIP, LEGS = 0.15, 0.5     # ... and legs: sprite in the outer 15% of the sides on half their height
BULK = 0.5              # or its sides are filled this much: a thing without legs, whatever its corners
FLAT_TOP = 0.97         # top fill from which a filled box has a real top (and may carry what it likes)
BULK_COVER = 0.8        # ... else it must account for this share of the sprite
RAGGED = 0.3            # share of a sprite's pixels on its outline from which it is a plant, not a thing
MIN_SIDE = 0.2          # units: narrower boxes are sheets or poles
MIN_RADIUS = 7          # px: narrower drums are poles
MIN_HEIGHT = 5          # px: lower solids are things lying on the floor
MIN_COVER = 0.55        # share of the sprite that the solid must account for
STEM_FILL = 0.5         # a stem and its foot fill no more of the space under the top
FEET_PX = 4             # a drum's outline this far under its arc is feet (a stove's legs): not error
LID_MIN_SEP = 4         # rows between the cap's bulge and the body's at least
LID_DIP_PX = 3          # the waist between the cap's bulge and the body's is at least this deep
LID_NECK = 0.6          # the waist carries at most this share of the cap's and body's width:
                        # a lid necks off the body (two blobs without a neck are not lid and body)
LID_FILL = 0.6          # the cap is a solid disc, not flames: it fills this much of its rows
LID_MAX_BODY = 0.65     # the cap is at most this high against the body's height
STEM_OFF = 0.3          # ... and stand no further from under its middle than this share of its width
RIM_PX = 3              # px under a top face that may still be its rim
LONG, SLICE = 1.5, 0.5  # units: a box at least this long is cut into slices about this long
SAME_PX = 4             # px: neighbouring slices whose heights differ by less are one box
CARVE_STRIPS = 8        # the carve fallback's length samples (task 120)
CARVE_STEP_PX = 8       # px: neighbouring strips whose heights differ by less are one band
CARVE_MIN_RUN = 2       # strips: a shorter band joins its closer neighbour
MIN_REST = 12           # px: fewer pixels outside the solid are not worth drawing
FAR = 1e6               # px: where a box is not


class Shape:
    """A sprite's opaque pixels with running sums down every column.

    x: screen x of the column centres, y grows downwards; both in px from the
    sprite's anchor. upper / lower: y of each column's top edge and of just
    below its lowest pixel.
    """

    def __init__(self, alpha, left, top):
        rows, columns = np.flatnonzero(alpha.any(axis=1)), np.flatnonzero(alpha.any(axis=0))
        self.alpha = alpha[rows[0]:rows[-1] + 1, columns[0]:columns[-1] + 1]
        self.left, self.top = left + int(columns[0]), top + int(rows[0])
        self.h, self.w = self.alpha.shape
        self.right, self.bottom = self.left + self.w, self.top + self.h
        self.x = self.left + np.arange(self.w) + 0.5
        self.some = self.alpha.any(axis=0)
        self.upper = self.top + np.argmax(self.alpha, axis=0).astype(float)
        self.lower = self.bottom - np.argmax(self.alpha[::-1], axis=0).astype(float)
        self.cum = np.vstack([np.zeros(self.w, int), np.cumsum(self.alpha, axis=0)])
        self.pixels = int(self.alpha.sum())
        padded = np.pad(self.alpha, 1)
        shut_in = padded[:-2, 1:-1] & padded[2:, 1:-1] & padded[1:-1, :-2] & padded[1:-1, 2:]
        self.ragged = 1 - (self.alpha & shut_in).sum() / self.pixels       # share of pixels on the outline

    def count(self, y0, y1):
        """(opaque pixels, pixels) of every column between y0 and y1 (arrays over the columns,
        or over heights x columns): the pixels whose centres lie in [y0, y1)."""
        r0 = np.ceil(y0 - self.top - 0.5).astype(int)
        r1 = np.maximum(np.ceil(y1 - self.top - 0.5).astype(int), r0)
        columns = np.arange(self.w)
        return self.cum[np.clip(r1, 0, self.h), columns] - self.cum[np.clip(r0, 0, self.h), columns], r1 - r0

    def off(self, edge, model):
        """Mean distance of an outline (upper or lower) from the model's, over the opaque columns."""
        return float(np.minimum(np.abs(edge - model)[self.some], CLIP_PX).mean())


class Fit:
    """A solid, in px from the sprite's anchor.

    kind        'box' or 'drum'
    centre      screen point of the middle of its top face
    a, b        box: its sides along u and v in units; drum: the half width of its ellipse in px (both)
    far, near   y of the top face's far and near edge at each of the shape's columns
    z0, z1      heights of its underside and its top (units)
    stem        it stands on a stem (its rest goes under it), not on the floor
    lid         a drum whose cap hovers over a gap: the top face is the lid; the
                game's own views keep the card, the solid is the fly camera's
    blocks      a box found from the floor: the boxes it is drawn as (_slices)
    top_fill, side_fill   share of its top face / of its sides that the sprite fills
    cover       share of the sprite inside its outline;  legs: see LEGS
    """

    def __init__(self, kind, shape, centre, a, b, far, near, z0, z1, stem=False, lid=False):
        self.kind, self.centre, self.a, self.b, self.far, self.near = kind, centre, a, b, far, near
        self.z0, self.z1, self.stem, self.along_u = z0, z1, stem, a >= b
        self.lid = lid
        depth = (z1 - z0) * proj.HPX
        inside, _ = shape.count(far, near + depth)
        top, area = shape.count(far, near)
        side, side_area = shape.count(near, near + depth)
        self.top_fill = top.sum() / max(area.sum(), 1)
        self.side_fill = side.sum() / max(side_area.sum(), 1)
        self.cover = inside.sum() / shape.pixels
        # Legs: sprite at both ends of the sides, on at least LEGS of their height.
        strip = max(4, int(LEG_STRIP * shape.w))
        rows = shape.top + np.arange(shape.h)[:, None] + 0.5
        sides = shape.alpha & (rows >= near[None, :]) & (rows < near[None, :] + depth)
        ends = [sides[:, :strip].any(axis=1).sum() / max(depth, 1), sides[:, -strip:].any(axis=1).sum() / max(depth, 1)]
        self.legs = min(ends) >= LEGS
        self.blocks = []

    def ground(self):
        """Ground offset from the sprite's anchor of the point under the middle of the top face."""
        return proj.delta_to_ground(self.centre[0], self.centre[1] + self.z1 * proj.HPX)

    def covers(self, shape):
        """Which pixels of shape.alpha's rectangle lie inside the solid's outline."""
        y = shape.top + np.arange(shape.h)[:, None] + 0.5
        if self.blocks:
            return np.any([(y >= block.far[None, :] - block.lift) & (y < block.near[None, :]) for block in self.blocks], axis=0)
        return (y >= self.far[None, :]) & (y < self.near[None, :] + (self.z1 - self.z0) * proj.HPX)

    def rest(self, shape):
        """Which pixels of shape.alpha the solid does not account for: (above it, under it)."""
        y = shape.top + np.arange(shape.h)[:, None] + 0.5
        below = shape.alpha & (y >= self.near[None, :] + (self.z1 - self.z0) * proj.HPX)
        return shape.alpha & ~self.covers(shape) & ~below, below

    def boxes(self, shape):
        """A box as the viewer draws it: [(ground offset of its middle from the sprite's anchor,
        a, b, z0, z1, wall along u, wall along v)].

        The walls: what the sprite shows above a box goes onto the planes of its two far sides,
        which rise to these heights for it (z1: not at all) - but only the outer sides of a row
        of boxes, and only where there is something to show.
        """
        blocks = self.blocks or [Block(shape, (self.centre[0] + (self.a * proj.SQ_U[0] + self.b * proj.SQ_V[0]) / 2,
                                               self.centre[1] + (self.a * proj.SQ_U[1] + self.b * proj.SQ_V[1]) / 2 + self.z1 * proj.HPX),
                                       self.a, self.b, self.z1 * proj.HPX)]
        above = self.rest(shape)[0]
        y = shape.top + np.arange(shape.h)[:, None] + 0.5
        out = []
        for n, block in enumerate(blocks):
            u, v = proj.delta_to_ground(*block.corner)
            peak = block.corner[0] + block.a * PX_U - block.b * PX_V             # screen x of the far corner
            rise = []                   # left of the far corner the far edge runs along u, right of it along v
            for columns, outer in ((shape.x < peak, self.along_u or n == len(blocks) - 1),
                                   (shape.x >= peak, not self.along_u or n == len(blocks) - 1)):
                over = above & (columns & block.on)[None, :] & (y < block.far[None, :] - block.lift)
                high = (block.far[None, :] - block.lift - y)[over]
                rise.append(block.lift + high.max() + 1 if outer and len(high) >= MIN_REST else block.lift)
            z0 = self.z0 if not self.blocks else 0.0
            out.append(((u - block.a / 2, v - block.b / 2), block.a, block.b, z0, block.lift / proj.HPX,
                        rise[0] / proj.HPX, rise[1] / proj.HPX))
        return out


class Block:
    """One box standing on the floor: the screen point of its near floor corner, its sides
    (units) and its height (px); far / near: the edges of its top face, were it on the floor,
    at the shape's columns (FAR where the box is not)."""

    def __init__(self, shape, corner, a, b, lift):
        self.corner, self.a, self.b, self.lift = corner, a, b, lift
        left, right = corner[0] - b * PX_V, corner[0] + a * PX_U
        self.near = np.minimum(corner[1] + V * (shape.x - corner[0]), corner[1] + U * (shape.x - corner[0]))
        self.far = np.maximum(corner[1] - b * proj.SQ_V[1] + U * (shape.x - left), corner[1] - a * proj.SQ_U[1] + V * (shape.x - right))
        self.on = (shape.x >= left) & (shape.x <= right)
        self.near[~self.on] = self.far[~self.on] = FAR


def _box(shape, corner):
    """The box whose foot has its near corner at screen point `corner` and reaches the shape's
    left and right edge, lying flat: a Block of height 0, or None if it has no width."""
    a, b = (shape.right - corner[0]) / PX_U, (corner[0] - shape.left) / PX_V
    return Block(shape, corner, a, b, 0) if min(a, b) >= MIN_SIDE else None


def _drum(shape, radius=None):
    """Half width, and the half height of its ellipse at the shape's columns."""
    if radius is None:
        radius = shape.w / 2
    return radius, K * radius * np.sqrt(np.clip(1 - ((shape.x - shape.left - radius) / radius) ** 2, 0, 1))


def _bulges(alpha):
    """The rows of the width profile's real bulges: local maxima a waist
    LID_DIP_PX deep separates, at least LID_MIN_SEP apart.

    A drum with a lid bulges twice: the cap's arc over the body's arc. A plain
    drum, a box or a pile bulge once."""
    rows = np.flatnonzero(alpha.any(axis=1))
    if len(rows) < 2 * LID_MIN_SEP:
        return []
    widths = alpha.sum(axis=1)[rows].astype(float)
    n = len(widths)
    cand, i = [], 0
    while i < n:
        near = widths[max(0, i - 1):min(n, i + 2)]
        if widths[i] >= near.max():
            j = i
            while j + 1 < n and widths[j + 1] == widths[i]:
                j += 1
            cand.append(((i + j) // 2, widths[i]))
            i = j + 1
        else:
            i += 1
    merged = []
    for row, w in cand:
        if merged and row - merged[-1][0] < LID_MIN_SEP:
            if w > merged[-1][1]:
                merged[-1] = (row, w)
        else:
            merged.append((row, w))
    peaks = [merged[0]]
    for row, w in merged[1:]:
        between = widths[peaks[-1][0] + 1:row]
        waist = between.min() if len(between) else w
        if waist <= min(peaks[-1][1], w) - LID_DIP_PX:
            peaks.append((row, w))
    return [int(rows[p[0]]) for p in peaks]


def _lid_drum(shape):
    """(outline error, Fit) of a drum whose body stands on feet under a lid cap, or None.

    The art tells it: the silhouette bulges twice (the cap over the body), the
    cap is a solid disc about the body's width and much lower than it, and the
    lower outline hugs the body's arc with nothing but feet under it. The raise
    climbs past the cap and the top face takes the lid; the game's own views
    keep the card (the gap under the lid is transparent there), so build_props
    makes this solid the fly camera's."""
    peaks = _bulges(shape.alpha)
    if len(peaks) < 2:
        return None
    rows = np.arange(shape.h)
    widths = shape.alpha.sum(axis=1)
    cap = peaks[0]
    body = max(peaks[1:], key=lambda p: widths[p])       # the body: the widest bulge under the cap
    waist = cap + int(np.argmin(widths[cap:body])) if body > cap + 1 else cap
    cap_rows = rows[rows <= waist]
    body_rows = rows[(rows > waist) & (rows <= body)]
    if len(body_rows) < LID_MIN_SEP * 2:
        return None
    radius = widths[body_rows].max() / 2
    body_h = rows[-1] - waist
    span_cols = np.flatnonzero(shape.alpha[cap_rows].any(axis=0))
    cap_fill = shape.alpha[cap_rows][:, span_cols].mean() if len(cap_rows) and len(span_cols) else 0.0
    if radius < MIN_RADIUS or body_h <= 0 \
            or not len(cap_rows) or widths[cap_rows].max() / 2 < MIN_RADIUS \
            or len(cap_rows) > LID_MAX_BODY * body_h \
            or widths[cap_rows].mean() / widths[body_rows].max() < 0.55 \
            or cap_fill < LID_FILL \
            or widths[waist] > LID_NECK * min(widths[cap], widths[body]):
        return None
    _, half = _drum(shape, radius)
    span = (shape.lower - half)[shape.some]
    foot = float(np.median(span))
    arc = foot + half
    hang = arc - shape.lower                      # the arc below the outline: the drum pokes out under it
    on = shape.lower - arc <= FEET_PX             # deeper than that the thing stands on feet, a stem
    if on.sum() < shape.some.sum() // 2:
        return None
    error = float(np.minimum(np.maximum(hang[on], 0), CLIP_PX).mean())
    lift = _raise(shape, foot - half, foot + half)
    if lift < MIN_HEIGHT or error > LID_OUTLINE_PX:
        return None
    fit = Fit("drum", shape, (shape.left + radius, foot - lift),
              radius, radius, foot - half - lift, foot + half - lift, 0.0, lift / proj.HPX, lid=True)
    return error, fit


def _raise(shape, far, near):
    """How many px above the floor the top face (far / near: its edges on the floor) fits best.

    Raising it by one row takes that row of the sprite into the solid; a top
    face that is not filled says the top is somewhere else.
    """
    lift = np.arange(0, max(1, int(near[near < FAR].max() - shape.top)) + 1)[:, None]
    inside, _ = shape.count(far[None, :] - lift, np.broadcast_to(near, (len(lift), shape.w)))
    top, area = shape.count(far[None, :] - lift, near[None, :] - lift)
    return int(np.argmax(inside.sum(axis=1) - EMPTY * (area - top).sum(axis=1)))


def _slices(shape, box):
    """A long box as a row of boxes, each as high as the sprite is there: [Block], near end first.

    The box is cut across its longer side into slices of about SLICE. Every
    slice reaches the long far side, where nothing of the object can stand
    behind it: there the sprite's upper outline is the slice's own, and how far
    it lies above the far edge of the foot is the slice's height. Neighbours
    of one height are joined.
    """
    along_u = box.a >= box.b
    length = box.a if along_u else box.b
    count = int(round(length / SLICE))
    if length < LONG or count < 2:
        return [box]
    size, step = length / count, (proj.SQ_U if along_u else proj.SQ_V)
    slices = []
    for n in range(count):                                      # the n-th slice from the near end
        corner = box.corner[0] - n * size * step[0], box.corner[1] - n * size * step[1]
        piece = Block(shape, corner, size if along_u else box.a, box.b if along_u else size, 0)
        peak = corner[0] + piece.a * PX_U - piece.b * PX_V
        side = piece.on & shape.some & ((shape.x < peak) if along_u else (shape.x >= peak))
        slices.append((corner, max(0.0, float(np.median((piece.far - shape.upper)[side]))) if side.any() else box.lift))
    blocks, first = [], 0
    for n in range(1, count + 1):
        if n == count or abs(slices[n][1] - slices[first][1]) > SAME_PX:
            run = (n - first) * size
            height = int(np.median([lift for _, lift in slices[first:n]]))
            if height >= MIN_HEIGHT:
                blocks.append(Block(shape, slices[first][0], run if along_u else box.a, box.b if along_u else run, height))
            first = n
    return blocks if len(blocks) > 1 else [box]


def real_directions(frames):
    """Directions whose frame is not the engine's UNUSED ART placeholder card
    (task 120): the card is opaque edge to edge, a real view's silhouette
    leaves its canvas open somewhere. -> the count of real directions."""
    return sum(1 for alpha in frames if real_view(alpha))


def real_view(alpha):
    """A direction's frame is a real view, not the placeholder card."""
    return alpha.any() and alpha.mean() < 0.97


CARVE_LONG = 1.35     # the carve's profile: its widest real view is at least
                      # this much wider than its tallest (123; the vehicles
                      # sit at 1.39-2.57, the leaning burnt frames at 0.13-1.02)


def real_aspect(frames):
    """The carve candidate's profile (task 123): the widest real direction's
    width over the tallest real direction's height. The game's big
    multi-quarter things - a car - read long and low; a leaning burnt frame
    does not, and the carve's ground boxes would lie it down."""
    widest, tallest = 0, 0
    for alpha in frames:
        if not real_view(alpha):
            continue
        rows, columns = np.flatnonzero(alpha.any(axis=1)), np.flatnonzero(alpha.any(axis=0))
        widest = max(widest, int(columns[-1] - columns[0] + 1))
        tallest = max(tallest, int(rows[-1] - rows[0] + 1))
    return widest / tallest if tallest else 0.0


def carved(shape, fit):
    """The forced box raised to what the art shows over its plan (task 120's
    carve fallback: a multi-view scenery art the plain fit refused - the
    game's big multi-quarter things, a car). The length cuts into strips,
    each as high as the sprite rises over the strip's far edge (the _slices
    measure at a car's shorter length), and neighbours of one height join:
    the bands are the body, the cabin. -> the fit with blocks set and z1 at
    the tallest band, or the fit unchanged when the strips hold nothing
    above the floor."""
    box = Block(shape, (fit.centre[0] + (fit.a * proj.SQ_U[0] + fit.b * proj.SQ_V[0]) / 2,
                        fit.centre[1] + (fit.a * proj.SQ_U[1] + fit.b * proj.SQ_V[1]) / 2 + fit.z1 * proj.HPX),
                fit.a, fit.b, fit.z1 * proj.HPX)
    along_u = fit.along_u
    size_step = proj.SQ_U if along_u else proj.SQ_V
    size = (fit.a if along_u else fit.b) / CARVE_STRIPS
    pieces = []
    for n in range(CARVE_STRIPS):
        corner = box.corner[0] - n * size * size_step[0], box.corner[1] - n * size * size_step[1]
        piece = Block(shape, corner, size if along_u else fit.a, fit.b if along_u else size, 0)
        pieces.append((corner, piece, corner[0] + piece.a * PX_U - piece.b * PX_V))
    strips = []
    for n, (corner, piece, peak) in enumerate(pieces):
        # the strip's own columns: between the previous strip's far corner and
        # its own (the outer boundary there is this strip's alone)
        if along_u:
            lo = box.corner[0] - fit.b * PX_V if n == 0 else pieces[n - 1][2]
            side = piece.on & shape.some & (shape.x > lo) & (shape.x <= peak)
        else:
            hi = box.corner[0] + fit.a * PX_U - fit.b * PX_V if n == 0 else pieces[n - 1][2]
            side = piece.on & shape.some & (shape.x >= peak) & (shape.x < hi)
        strips.append((corner, max(0.0, float(np.median((piece.far - shape.upper)[side]))) if side.any() else 0.0))
    bands = []                                              # [first strip, last strip, height]
    for n, (_, height) in enumerate(strips):
        if bands and abs(height - bands[-1][2]) <= CARVE_STEP_PX:
            bands[-1][1] = n
            bands[-1][2] = float(np.mean([h for _, h in strips[bands[-1][0]:n + 1]]))
        else:
            bands.append([n, n, height])
    for n in range(len(bands) - 1, -1, -1):                 # a short band joins the closer one
        if len(bands) > 1 and bands[n][1] - bands[n][0] + 1 < CARVE_MIN_RUN:
            if n + 1 >= len(bands) or (n > 0 and abs(bands[n][2] - bands[n - 1][2]) <= abs(bands[n][2] - bands[n + 1][2])):
                other = bands[n - 1]
            else:
                other = bands[n + 1]
            lo, hi = min(bands[n][0], other[0]), max(bands[n][1], other[1])
            other[0], other[1] = lo, hi
            other[2] = float(np.mean([h for _, h in strips[lo:hi + 1]]))
            bands.pop(n)
    blocks = []
    for first, last, height in bands:
        if height < MIN_HEIGHT:
            continue
        corner = box.corner[0] - first * size * size_step[0], box.corner[1] - first * size * size_step[1]
        run = (last - first + 1) * size
        blocks.append(Block(shape, corner, run if along_u else fit.a, fit.b if along_u else run, height))
    if not blocks:
        return fit
    fit.blocks = blocks
    fit.z1 = max(block.lift for block in blocks) / proj.HPX
    return fit


def _from_floor(shape, force=False, allow_lid=False):
    """Solids standing on the floor: [(outline error, Fit)] for a box and a drum, where they exist.
    force: "box" or "drum", that solid wherever one stands at all (an exception by name, overrides.json).
    allow_lid: the lid-drum candidate joins the pick (the loose clutter a lid vouches for; task 119)."""
    out = []
    steps = shape.x[shape.some], shape.lower[shape.some]
    cv, cu = walls._kth(steps[1] - V * steps[0], True), walls._kth(steps[1] - U * steps[0], True)
    corner = (cu - cv) / (V - U), (V * cu - U * cv) / (V - U)       # where the two supporting lines meet
    flat = _box(shape, corner)
    if flat:
        a, b = flat.a, flat.b
        lift = flat.lift = _raise(shape, flat.far, flat.near)
        # The three near corners of the foot must be there: the sprite comes down to them.
        corners = [(shape.left, corner[1] - b * proj.SQ_V[1]), corner, (shape.right, corner[1] - a * proj.SQ_U[1])]
        ys, xs = np.nonzero(shape.alpha)
        found = all(np.hypot(xs + shape.left + 0.5 - x, ys + shape.top + 0.5 - y).min() <= CORNER_PX for x, y in corners)
        centre = (corner[0] + (a * proj.SQ_U[0] + b * proj.SQ_V[0]) / -2, corner[1] - (a * proj.SQ_U[1] + b * proj.SQ_V[1]) / 2 - lift)
        box = Fit("box", shape, centre, a, b, flat.far - lift, flat.near - lift, 0.0, lift / proj.HPX)
        # A thing with a flat top (a table, a bed) may carry and hide much of itself; without
        # one, a box that leaves a good part of the sprite above its "top" is the foot of a heap.
        bulk = box.side_fill >= BULK
        furniture = found and box.legs and (box.top_fill >= FLAT_TOP or not bulk)
        if force == "box" and lift >= MIN_HEIGHT:
            box.blocks = [flat]
            return [(shape.off(shape.lower, flat.near), box)]
        if lift >= MIN_HEIGHT and (furniture or bulk) and box.cover >= (MIN_COVER if furniture else BULK_COVER):
            box.blocks = _slices(shape, flat) if bulk else [flat]       # what stands on a table is not the table
            out.append((shape.off(shape.lower, flat.near), box))
    radius, half = _drum(shape)
    if radius >= MIN_RADIUS:
        foot = walls._kth((shape.lower - half)[shape.some], True)
        lift = _raise(shape, foot - half, foot + half)
        error = shape.off(shape.lower, foot + half)
        if lift >= MIN_HEIGHT and (error <= OUTLINE_PX or force == "drum"):
            centre = (shape.left + radius, foot - lift)
            drum = Fit("drum", shape, centre, radius, radius, foot - half - lift, foot + half - lift, 0.0, lift / proj.HPX)
            if force == "drum":
                return [(error, drum)]
            out.append((error, drum))
    if allow_lid and force != "drum":
        lid = _lid_drum(shape)
        if lid:
            out.append(lid)
    # A top with holes will do where the solid is the whole sprite (frames, open boxes, a stove).
    return [(error, fit) for error, fit in out if fit.cover >= MIN_COVER
            and fit.top_fill >= (OPEN_TOP_FILL if fit.cover >= WHOLE and fit.side_fill < BULK else TOP_FILL)]


def _from_top(shape):
    """Solids on a stem: [(outline error, Fit)]."""
    out = []
    steps = shape.x[shape.some], shape.upper[shape.some]
    cl, cr = walls._kth(steps[1] - U * steps[0], False), walls._kth(steps[1] - V * steps[0], False)
    peak = (cl - cr) / (V - U), (V * cl - U * cr) / (V - U)         # the far corner of the top face
    a, b = (peak[0] - shape.left) / PX_U, (shape.right - peak[0]) / PX_V
    candidates = []
    if min(a, b) >= MIN_SIDE:
        corner = peak[0] + a * proj.SQ_U[0] + b * proj.SQ_V[0], peak[1] + a * proj.SQ_U[1] + b * proj.SQ_V[1]
        far = np.maximum(U * shape.x + cl, V * shape.x + cr)
        near = np.minimum(corner[1] + V * (shape.x - corner[0]), corner[1] + U * (shape.x - corner[0]))
        candidates.append(("box", ((peak[0] + corner[0]) / 2, (peak[1] + corner[1]) / 2), a, b, far, near))
    radius, half = _drum(shape)
    if radius >= MIN_RADIUS:
        middle = walls._kth((shape.upper + half)[shape.some], False)
        candidates.append(("drum", (shape.left + radius, middle), radius, radius, middle - half, middle + half))
    for kind, centre, a, b, far, near in candidates:
        error = shape.off(shape.upper, far)
        # The solid hangs from its top as far down as it stays filled from side to side.
        drop = np.arange(0, max(1, int(shape.bottom - near.min())) + 1)[:, None]
        side, area = shape.count(np.broadcast_to(near, (len(drop), shape.w)), near[None, :] + drop)
        depth = int(np.argmax(side.sum(axis=1) - EMPTY * (area - side).sum(axis=1)))
        # The stem: what is left under it. Its foot (the lowest rows) is a small circle on the floor.
        foot = np.flatnonzero(shape.alpha[-3:].any(axis=0))
        foot_x, foot_half = shape.left + (foot[0] + foot[-1] + 1) / 2, (foot[-1] - foot[0] + 1) / 2
        z1 = (shape.bottom - K * foot_half - centre[1]) / proj.HPX
        if error > OUTLINE_PX or abs(foot_x - centre[0]) > STEM_OFF * shape.w:
            continue
        stem = z1 * proj.HPX - depth >= MIN_HEIGHT
        if not stem:                                            # it stands on its foot
            z1 = depth / proj.HPX
        fit = Fit(kind, shape, centre, a, b, far, near, max(z1 - depth / proj.HPX, 0.0), z1, stem)
        under = shape.alpha[max(0, int(near.max() + depth + RIM_PX - shape.top)):]      # clear of the top's rim
        if fit.top_fill >= STEM_TOP_FILL and (under.size and under.mean() <= STEM_FILL if stem else depth >= MIN_HEIGHT):
            out.append((error, fit))
    return out


def fit(alpha, left, top, force=False, allow_lid=False):
    """The solid for a sprite (its opaque pixels without the floor's shade, light.outer_dark;
    left / top: px of its first pixel from the anchor), or None if it had better stay a card.
    force: "box" or "drum" whenever one stands (chairs and crates are as ragged as plants).
    allow_lid: a drum with a lid may win the pick (the mapper's loose clutter only: stood up,
    a lid's gap is transparent in the game's own views, so those keep the card; task 119).
    -> (Shape, Fit)"""
    if not alpha.any():
        return None
    shape = Shape(alpha, left, top)
    if force:
        found = _from_floor(shape, force=force, allow_lid=allow_lid)
        return (shape, found[0][1]) if found else None
    if shape.ragged > RAGGED:
        return None
    hung = _from_top(shape)
    floor = _from_floor(shape, allow_lid=allow_lid)
    if hung:
        floor = [pair for pair in floor if not pair[1].lid]
    else:
        lids = [pair for pair in floor if pair[1].lid]
        if lids:
            # A round body under a lid, standing on the floor: the box's straight
            # lines contradict the art the drum's arc just vouched for, so the
            # lid drum takes it whatever the box's foot line scored.
            return shape, min(lids, key=lambda pair: pair[0])[1]
    for found in (floor, hung):
        if found:
            return shape, min(found, key=lambda pair: pair[0])[1]
    return None


# ------------------------------------------------------------------ composites
SEAM_PX = 12            # sprites that meet along a straight vertical line this long are slices of one picture
WITHIN = 0.95           # share of a sprite inside the bounds of a bigger one of its name: a detail of that thing


class Piece:
    """One map object's sprite on the game's screen.

    index / rgba: its frame as palette indices and as colours; (x, y): world
    pixel of the frame's top-left corner; order: when the game paints it;
    board / sheet: the row of the scene's boards or walls that shows it now.
    """

    def __init__(self, obj, index, rgba, x, y, order, name, board=None, sheet=None):
        self.obj, self.index, self.rgba, self.x, self.y, self.order, self.name = obj, index, rgba, x, y, order, name
        self.board, self.sheet = board, sheet
        columns = np.flatnonzero((index != 0).any(axis=0))
        rows = np.flatnonzero((index != 0).any(axis=1))
        self.left, self.right = x + int(columns[0]), x + int(columns[-1]) + 1
        self.bounds = self.left, y + int(rows[0]), self.right, y + int(rows[-1]) + 1     # of its opaque pixels
        self.pixels = int((index != 0).sum())

    def edge(self, right):
        """World rows in which the sprite's last (or first) column is opaque."""
        return self.y + np.flatnonzero(self.index[:, (self.right - 1 if right else self.left) - self.x])


def groups(pieces, details=True):
    """Sort pieces into things: [[Piece]].

    Big scenery was cut into slices, one map object each (long tables, bunk
    beds, machines): two sprites belong together when one ends in the screen
    column where the other begins and their edge columns touch for SEAM_PX rows
    - nothing but a cut is that straight. So do objects of one name on one hex
    (the housing and the wheel of Vault 13's door), and objects of one name
    whose smaller sprite lies within the bigger one's bounds (a bed's rail or a
    bookshelf's lower half painted as an object of its own): fitted alone, such
    a detail became a plank in mid-air (details=False: not the details).
    """
    parent = list(range(len(pieces)))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    starts, places = {}, {}
    for i, piece in enumerate(pieces):
        starts.setdefault(piece.left, []).append(i)
        places.setdefault((piece.obj.tile, piece.name), []).append(i)
    for i, piece in enumerate(pieces):
        for j in starts.get(piece.right, ()):
            if len(np.intersect1d(piece.edge(True), pieces[j].edge(False))) >= SEAM_PX:
                parent[find(i)] = find(j)
    for same in places.values():
        for i in same[1:]:
            parent[find(i)] = find(same[0])
    named = {}
    for i, piece in enumerate(pieces if details else ()):
        named.setdefault(piece.name, []).append(i)
    for same in named.values():
        for i in same:
            for j in same:
                if pieces[j].pixels < pieces[i].pixels or (pieces[j].pixels == pieces[i].pixels and j <= i):
                    continue
                little, big = pieces[i], pieces[j]           # is the smaller one a detail of the bigger?
                if detail(little, big):
                    parent[find(i)] = find(j)
    out = {}
    for i, piece in enumerate(pieces):
        out.setdefault(find(i), []).append(piece)
    return [sorted(group, key=lambda piece: piece.order) for group in out.values()]


def detail(little, big):
    """Whether a piece lies within the bounds of a bigger one of its name, a detail of that thing."""
    x0, y0, x1, y1 = big.bounds
    if little.name != big.name or little.sheet is not None or big.sheet is not None or little.bounds[0] >= x1 or little.bounds[2] <= x0 \
            or little.bounds[1] >= y1 or little.bounds[3] <= y0:
        return False
    ys, xs = np.nonzero(little.index)
    xs, ys = xs + little.x, ys + little.y
    return bool(np.mean((xs >= x0) & (xs < x1) & (ys >= y0) & (ys < y1)) >= WITHIN)


def lead(group):
    """The piece of a group that all the others are details of (a bed and its rail), or None."""
    big = max(group, key=lambda piece: piece.pixels)
    return big if all(piece is big or detail(piece, big) for piece in group) else None


def compose(pieces):
    """Several pieces as the one picture the game paints: (index, rgba, x, y)."""
    x, y = min(p.x for p in pieces), min(p.y for p in pieces)
    width = max(p.x + p.index.shape[1] for p in pieces) - x
    height = max(p.y + p.index.shape[0] for p in pieces) - y
    index, rgba = np.zeros((height, width), pieces[0].index.dtype), np.zeros((height, width, 4), np.uint8)
    for p in pieces:
        window = (slice(p.y - y, p.y - y + p.index.shape[0]), slice(p.x - x, p.x - x + p.index.shape[1]))
        index[window][p.index != 0] = p.index[p.index != 0]
        rgba[window][p.index != 0] = p.rgba[p.index != 0]
    return index, rgba, x, y


def fill_holes(rgba):
    """The texture of a solid's faces with its transparent texels filled in the
    colour of the nearest opaque ones, an average of the neighbours, so no face
    shows the scene through the art's holes (the owner's rule of no holes in
    the 3D things, 115). Two classes fill: the texels enclosed on all sides,
    and (121's pass) the ones with opaque art both above and below in their
    column - a band the shape straddles, the gap a hovering lid leaves over a
    body, which the border-reach rule kept open but a 3D face shows as a slit
    straight through the thing. The open border, the ground round the feet and
    the painted shade band, stays as it is: a cut-out that the art holds open
    to the sky keeps the original's pixels."""
    alpha = rgba[..., 3] > 0
    if alpha.all() or not alpha.any():
        return rgba
    # Transparent texels the border reaches stay open; only enclosed ones fill.
    reach = np.zeros_like(alpha)
    reach[0, :] = reach[-1, :] = reach[:, 0] = reach[:, -1] = True
    reach &= ~alpha
    while True:
        grown = reach.copy()
        grown[1:, :] |= reach[:-1, :]
        grown[:-1, :] |= reach[1:, :]
        grown[:, 1:] |= reach[:, :-1]
        grown[:, :-1] |= reach[:, 1:]
        grown &= ~alpha
        if (grown == reach).all():
            break
        reach = grown
    holes = ~alpha & ~reach
    # A band the shape straddles: transparent, with opaque art above and below
    # in the same column that belong to DIFFERENT pieces of the art - the gap a
    # hovering lid leaves over its body. A bite out of one piece (a notch open
    # to the side) stays: its above and below are the same connected piece.
    label = np.zeros(alpha.shape, np.int32)
    remaining = alpha.copy()
    piece = 0
    while remaining.any():
        piece += 1
        r, c = np.argwhere(remaining)[0]
        front = np.zeros_like(remaining)
        front[r, c] = True
        while True:
            grown = front.copy()
            grown[1:, :] |= front[:-1, :]
            grown[:-1, :] |= front[1:, :]
            grown[:, 1:] |= front[:, :-1]
            grown[:, :-1] |= front[:, 1:]
            grown &= remaining
            if (grown == front).all():
                break
            front = grown
        label[front] = piece
        remaining &= ~front
    nearest_above = np.zeros(alpha.shape, np.int32)
    carry = np.zeros(alpha.shape[1], np.int32)
    for r in range(alpha.shape[0]):
        nearest_above[r] = carry
        carry = np.where(alpha[r], label[r], carry)
    nearest_below = np.zeros(alpha.shape, np.int32)
    carry = np.zeros(alpha.shape[1], np.int32)
    for r in range(alpha.shape[0] - 1, -1, -1):
        nearest_below[r] = carry
        carry = np.where(alpha[r], label[r], carry)
    holes = holes | (~alpha & (nearest_above > 0) & (nearest_below > 0)
                     & (nearest_above != nearest_below))
    if not holes.any():
        return rgba
    out = rgba.copy()
    colours = out[..., :3].astype(np.uint32)
    filled = alpha
    todo = holes
    while todo.any():
        up, down = np.zeros_like(filled), np.zeros_like(filled)
        up[1:, :] = filled[:-1, :]
        down[:-1, :] = filled[1:, :]
        left, right = np.zeros_like(filled), np.zeros_like(filled)
        left[:, 1:] = filled[:, :-1]
        right[:, :-1] = filled[:, 1:]
        count = up.astype(int) + down + left + right
        newly = todo & (count > 0)
        if not newly.any():
            break
        total = np.zeros((*newly.shape, 3), np.uint32)
        for neighbour, mask in ((np.roll(colours, 1, 0), up), (np.roll(colours, -1, 0), down),
                                (np.roll(colours, 1, 1), left), (np.roll(colours, -1, 1), right)):
            total += neighbour * mask[..., None]
        cols = (total[newly] + count[newly, None] // 2) // count[newly, None]
        out[..., :3][newly] = cols.astype(np.uint8)
        out[..., 3][newly] = 255
        colours = out[..., :3].astype(np.uint32)
        filled = filled | newly
        todo = todo & ~newly
    return out
