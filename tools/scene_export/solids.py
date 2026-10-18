"""Furniture as solids, the way the donor viewer textures them (render.js solid()).

The exporter (vendor fp/props.py) gives deep scenery a box or a drum in place
of its sprite card. A point of the solid shows the pixel it lands on in the
sprite from the game's camera, so the game's view still sees the sprite and
the 3D view sees a thing with depth: a bed lies on the floor, a shelf stands
square to the walls. These are the faces the game's camera sees; the far sides
(mirror images) are first person only in the donor and hidden here anyway.

As in the donor's game view, a solid's faces take the depth of its card: the
upright plane of its hex row (a hair nearer), reached along the game's view
ray. The solid gives the shape; what hides what stays the game's own painting
order, so a woman beside a bed and a headboard against a wall stay whole.

What a solid does not cover of a drum or a stem (`rests`) stays a sprite card,
static here: its depth is that of its hex row's plane, reached along the
game's view ray (row_plane), like a live card.
"""

from __future__ import annotations

import math

DRUM_SIDES = 16
OVER_CARD = 2e-5    # units a solid is nearer than its card's depth plane (render.js): it wins the tie


class Projection:
    """px(u, v, z) = origin + u * squ + v * sqv - (0, hpx * z), y down."""

    def __init__(self, raw):
        self.squ, self.sqv, self.origin = raw["squ"], raw["sqv"], raw["origin"]
        self.hpx, self.bbpx = raw["hpx"], raw["bbPx"]
        self.row_tilt, self.stack = raw["rowTilt"], raw["stack"]
        self.det = self.squ[0] * self.sqv[1] - self.sqv[0] * self.squ[1]
        ray = (self.sqv[0], -self.squ[0], (self.sqv[0] * self.squ[1] - self.squ[0] * self.sqv[1]) / self.hpx)
        length = math.sqrt(sum(c * c for c in ray))
        self.toward = tuple(c / length for c in ray)        # (u, v, z) towards the game's camera

    def ground_to_px(self, u, v, z=0.0):
        return (self.origin[0] + u * self.squ[0] + v * self.sqv[0],
                self.origin[1] + u * self.squ[1] + v * self.sqv[1] - self.hpx * z)

    def delta_to_ground(self, dx, dy):
        return ((self.sqv[1] * dx - self.sqv[0] * dy) / self.det,
                (self.squ[0] * dy - self.squ[1] * dx) / self.det)

    def px_to_ground(self, x, y):
        return self.delta_to_ground(x - self.origin[0], y - self.origin[1])

    def ground_dir(self, dx, dy):
        u, v = self.delta_to_ground(dx, dy)
        n = math.hypot(u, v)
        return u / n, v / n


def hex_px(tile):
    hx, hy = tile % 200, tile // 200
    a = 199 - hx
    return 48 * (a >> 1) + 32 * (a & 1) + 16 * hy + 16, -12 * (a >> 1) + 12 * hy + 8


def hex_from_px(x, y):
    """Tile whose hexagon holds world pixel (x, y), or -1 (tileFromScreenXY)."""
    x, y = math.floor(x), math.floor(y)
    row = y // 12
    shifted = x - 16 * row
    band = shifted // 64
    tile_row, local, column = band + row, shifted - band * 64, 2 * band
    if local >= 32:
        local -= 32
        column += 1
    mx, my = int(local), int(y - row * 12)
    corner = 0
    if my < 4:
        if 64 - 4 * mx > 16 * my and mx < 16:
            corner = 1
        elif mx >= 16 and 4 * (mx - 16) > 16 * my:
            corner = 2
    if corner == 2:
        column += 1
        if column & 1:
            tile_row -= 1
    elif corner == 1:
        tile_row -= 1
    hx = 199 - column
    return tile_row * 200 + hx if 0 <= hx < 200 and 0 <= tile_row < 200 else -1


def row_plane(proj, u, v, stack=0):
    """v of the upright plane its hex row's sprites are sorted at (geom.js rowPlane)."""
    tile = hex_from_px(*proj.ground_to_px(u, v))
    if tile < 0:
        return v
    hx = tile % 200
    nudge = proj.row_tilt * hx + proj.stack * min(stack, proj.row_tilt / proj.stack - 1)
    return proj.px_to_ground(*hex_px(tile - (hx & 1)))[1] + nudge


def clip(corners, x, y, w, h):
    """The part of a convex polygon [u, v, z, tx, ty] whose texels lie in the rectangle."""
    for axis, limit, sign in ((3, x, 1), (3, x + w, -1), (4, y, 1), (4, y + h, -1)):
        kept = []
        for i, p in enumerate(corners):
            q = corners[(i + 1) % len(corners)]
            dp, dq = (p[axis] - limit) * sign, (q[axis] - limit) * sign
            if dp >= 0:
                kept.append(p)
            if dp * dq < 0:
                kept.append([c + (q[k] - c) * dp / (dp - dq) for k, c in enumerate(p)])
        corners = kept
    return corners


def depth_home(proj, corner, plane):
    """The way along the game's view ray from a corner to the depth plane v = plane."""
    t = (plane - corner[1]) / proj.toward[1]
    return [proj.toward[0] * t, proj.toward[1] * t, proj.toward[2] * t]


def solid_faces(proj, sprites, boards, row):
    """(page, polygon) of each face the game's camera sees of one scene.props row;
    polygon corners are [u, v, z, atlas x, atlas y, home u, home v, home z],
    clipped to the sprite. A solid that stands for several cards (board -1)
    keeps its own depth."""
    s, u, v, board, round_, cu, cv, a, b, z0, z1, wall_u, wall_v = row[:13]
    plane = None
    if board >= 0:
        _, bu, bv, stack = boards[board][:4]
        plane = row_plane(proj, bu, bv, stack) + OVER_CARD
    page, x, y, w, h, ox, oy = sprites[s]
    ax, ay = proj.ground_to_px(u, v)

    def texel_at(point):
        px, py = proj.ground_to_px(*point)
        return [x + px - ax - ox, y + py - ay - oy]

    faces = []

    def face(points, from_points=None):
        shows = from_points or points
        corners = clip([[*point, *texel_at(shows[i])] for i, point in enumerate(points)], x, y, w, h)
        if len(corners) >= 3:
            faces.append((page, [corner + (depth_home(proj, corner, plane) if plane is not None else [0.0, 0.0, 0.0])
                                 for corner in corners]))

    if round_:
        # The cylinder over the floor ellipse a x b px on the game's screen: only the
        # quarter facing the camera shows what it lands on; the others repeat it
        # mirrored at every seam. Taking its card's depth, a drum is drawn as the
        # game's view does: that quarter and the top. The others would tie with
        # them in depth and show their mirror images (a cable spool looked torn).
        quarter = DRUM_SIDES // 4

        def rim(i, z):
            angle = 2 * math.pi * i / DRUM_SIDES
            du, dv = proj.delta_to_ground(a * math.cos(angle), b * math.sin(angle))
            return [cu + du, cv + dv, z]

        def side(i, j):
            return [rim(i, z0), rim(j, z0), rim(j, z1), rim(i, z1)]

        for i in range(DRUM_SIDES):
            step = (i + DRUM_SIDES - quarter // 2) % DRUM_SIDES
            turn, within = divmod(step, quarter)

            def source(n):
                return quarter / 2 + (quarter - n if turn % 2 else n)

            if turn == 0 or plane is None:
                face(side(i, i + 1), side(source(within), source(within + 1)))
        face([rim(i, z1) for i in range(DRUM_SIDES)])
        return faces

    u0, u1, v0, v1 = cu - a / 2, cu + a / 2, cv - b / 2, cv + b / 2

    def along_u(vv, low, high):
        return [[u0, vv, low], [u1, vv, low], [u1, vv, high], [u0, vv, high]]

    def along_v(uu, low, high):
        return [[uu, v0, low], [uu, v1, low], [uu, v1, high], [uu, v0, high]]

    face([[u0, v0, z1], [u1, v0, z1], [u1, v1, z1], [u0, v1, z1]])
    face(along_u(v1, z0, z1))
    face(along_v(u1, z0, z1))
    # What rises behind the top (a headboard): on the far sides, seen from inside.
    if wall_u > z1:
        face(along_u(v0, z1, wall_u))
    if wall_v > z1:
        face(along_v(u0, z1, wall_v))
    return faces


def rest_card(proj, sprites, row):
    """(page, corners) of one scene.rests row as a game-view card: corners
    [u, v, z, atlas x, atlas y, home u, home v, home z], home = the way along
    the game's view ray to its hex row's depth plane."""
    s, u, v = row[:3]
    page, x, y, w, h, ox, oy = sprites[s]
    top, bottom = oy, oy + h
    if bottom <= 0:
        shift = lean = 0.0
        z_bottom, z_top = -bottom / proj.hpx, -top / proj.hpx
    else:
        far = min(bottom, max(0, top))
        shift, lean = (bottom + far) / 2, (bottom - far) / 2
        z_bottom, z_top = 0.0, (far - top) / proj.hpx
    du, dv = proj.delta_to_ground(0, shift)
    down_u, down_v = proj.delta_to_ground(0, 1)
    right_u, right_v = proj.ground_dir(1, 0)
    base_u, base_v = u + du, v + dv
    plane = row_plane(proj, u, v)
    corners = []
    for i, j in ((0, 1), (1, 1), (1, 0), (0, 0)):
        along = (ox + i * w) / proj.bbpx
        towards = lean if j else -lean
        cu = base_u + right_u * along + down_u * towards
        cv = base_v + right_v * along + down_v * towards
        cz = z_bottom if j else z_top
        corners.append([cu, cv, cz, x + i * w, y + j * h, *depth_home(proj, [cu, cv, cz], plane)])
    return page, corners


def screen_area(proj, corners):
    """Twice the signed area of a polygon on the game's screen, y up."""
    points = [proj.ground_to_px(c[0], c[1], c[2]) for c in corners]
    area = 0.0
    for i, (px, py) in enumerate(points):
        qx, qy = points[(i + 1) % len(points)]
        area += px * (-qy) - qx * (-py)
    return area
