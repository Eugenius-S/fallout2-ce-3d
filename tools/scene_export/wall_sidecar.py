"""Versioned donor-surface triangles for the CE static wall preview."""

from __future__ import annotations

import math
import struct

from scene_check import one_map_elevation
from solids import Projection, rest_card, rest_cross, screen_area, solid_faces


MAGIC = b"F2WL"
VERSION = 5
# magic, version, header bytes, atlas pages, batches, triangles, objects
HEADER = struct.Struct("<4sHHIIII")
# atlas page, width, height, triangles, flags. A page has up to two batches:
# what every camera draws, and what only a free camera does (FLY_ONLY).
BATCH = struct.Struct("<IIIII")
FLY_ONLY = 1
GAME_ONLY = 2  # what the fly camera draws its own way (a drum's near half)
# GL position xyz, top-down normalized atlas uv, GL point whose depth the vertex
# takes (itself, except on wall tops: the front-plane point its texel was painted
# on), the map object it shows (its row in the object table after the batches;
# -1: none), so the engine can cut the see-through circle out of it
VERTEX = struct.Struct("<9f")
FLOATS = 9
OBJECT = struct.Struct("<iI")  # map object tile, fid
GL_MAP_OFFSET = 50.0


def _finite(value, label):
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
        raise ValueError(f"{label} must be a finite number")
    return float(value)


def _int(value, label):
    if isinstance(value, bool) or not isinstance(value, int):
        raise ValueError(f"{label} must be an integer")
    return value


def _record(row, length, label):
    if not isinstance(row, list) or len(row) < length:
        raise ValueError(f"invalid {label} record")
    return row


class _Batches(dict):
    """Vertex lists by atlas page; `object` is the object row the faces added
    now belong to."""
    object = -1.0


def _quad(batches, page, page_size, corners):
    """Append the same a,b,c / a,c,d triangulation as donor Mesh.quad. A corner
    may carry the donor's `home`: the way back to the point whose depth it takes."""
    width, height = page_size
    points = []
    for corner in corners:
        u, v, z, tx, ty = corner[:5]
        hu, hv, hz = corner[5:8] if len(corner) > 5 else (0.0, 0.0, 0.0)
        u, v, z, hu, hv, hz = (_finite(n, "wall position") for n in (u, v, z, hu, hv, hz))
        tx, ty = (_finite(n, "wall atlas coordinate") for n in (tx, ty))
        if not (0 <= tx <= width and 0 <= ty <= height):
            raise ValueError("wall texture coordinates are outside the sprite atlas")
        points.append((u - GL_MAP_OFFSET, z, v - GL_MAP_OFFSET, tx / width, ty / height,
                       u + hu - GL_MAP_OFFSET, z + hz, v + hv - GL_MAP_OFFSET, batches.object))
    target = batches.setdefault(page, [])
    for index in (0, 1, 2, 0, 2, 3):
        target.extend(points[index])


def _upright(batches, sprites, pages, sprite_id, ua, va, ub, vb, z0, z1, tx, ty, tw, th, part=(0.0, 1.0)):
    """The picture's rectangle upright between ground points a and b; `part`
    keeps only that stretch of it (fractions from a to b)."""
    sprite_id = _int(sprite_id, "sprite index")
    if not 0 <= sprite_id < len(sprites):
        raise ValueError(f"sprite index {sprite_id} is outside scene.sprites")
    page, x, y, w, h, *_ = sprites[sprite_id]
    page = _int(page, "sprite page")
    if not 0 <= page < len(pages):
        raise ValueError(f"sprite page {page} is outside scene.pages")
    x, y, w, h = (_int(value, "sprite rectangle") for value in (x, y, w, h))
    tx, ty, tw, th = (_finite(value, "sprite subrectangle") for value in (tx, ty, tw, th))
    if w <= 0 or h <= 0:
        raise ValueError("sprite rectangle must have positive dimensions")
    image_w, image_h = pages[page]
    if x < 0 or y < 0 or x + w > image_w or y + h > image_h:
        raise ValueError("sprite rectangle is outside its atlas page")
    # Donor wallRest deliberately reuses the front width for a separately packed
    # back/above sprite; validate the sampled rectangle against the actual page.
    if not (0 <= tx and 0 <= ty and tx + tw <= image_w and ty + th <= image_h):
        raise ValueError("sprite subrectangle is outside its atlas page")
    f0, f1 = part
    if f1 <= f0:
        return
    pa, pb = (ua + (ub - ua) * f0, va + (vb - va) * f0), (ua + (ub - ua) * f1, va + (vb - va) * f1)
    xa, xb = tx + tw * f0, tx + tw * f1
    _quad(batches, page, (image_w, image_h), [
        [*pa, z0, xa, ty + th], [*pb, z0, xb, ty + th],
        [*pb, z1, xb, ty], [*pa, z1, xa, ty],
    ])


# Feet (task 081). Some wall pictures end a little above the floor: their
# clear bottom rows were trimmed, and the game shows the floor there. The
# free camera saw such a wall float over the street, and at a corner the
# core stand under it alone. A face of a thick wall whose bottom is at most
# FOOT_MAX above the floor runs on down to it for the free camera, in its
# picture's bottom row; a higher one stands on something else.
FOOT_MIN = 0.005
FOOT_MAX = 0.5


FOOT_SCAN = 8  # picture rows above the bottom a foot may take its texels from
FOOT_SOLID = 0.95  # ... the lowest this opaque (a clear bottom row would stripe the foot)


def _foot_z(row):
    """The bottom of a thick wall row's faces when they need a foot, else None."""
    z0 = _finite(row[5], "wall height")
    return z0 if row[10] >= CORE_THICK and FOOT_MIN < z0 <= FOOT_MAX else None


def _foot_row(atlas, page, x0, x1, y_bottom):
    """The atlas row (its middle) a foot under picture columns x0 .. x1 whose
    bottom is y_bottom takes: of the lowest FOOT_SCAN rows the lowest
    FOOT_SOLID opaque, else the most opaque (the bottom row without the atlas)."""
    if atlas is None:
        return y_bottom - 0.5
    x0, x1 = sorted((x0, x1))
    bottom = int(round(y_bottom))
    best = None
    for y in range(bottom - 1, max(bottom - 1 - FOOT_SCAN, -1), -1):
        share = float((atlas[page][y, int(x0):max(int(math.ceil(x1)), int(x0) + 1), 3] >= 128).mean())
        if share >= FOOT_SOLID:
            return y + 0.5
        if best is None or share > best[0]:
            best = (share, y + 0.5)
    return best[1] if best else y_bottom - 0.5


def _clip(polygon, side):
    """The part of a convex polygon of 3D points where side(point) >= 0; side is
    affine on the polygon's plane, so cut points interpolate exactly."""
    out = []
    for index, point in enumerate(polygon):
        nxt = polygon[(index + 1) % len(polygon)]
        a, b = side(point), side(nxt)
        if a >= 0:
            out.append(point)
        if (a >= 0) != (b >= 0):
            f = a / (a - b)
            out.append(tuple(p + (q - p) * f for p, q in zip(point, nxt)))
    return out


def _split(polygons, side):
    return [part for polygon in polygons
            for part in (_clip(polygon, side), _clip(polygon, lambda p: -side(p))) if len(part) >= 3]


def _wall_back(batches, sprites, pages, row, hpx, keep=(0.0, 1.0), atlas=None):
    """What the game's camera does not see of a solid wall (donor wallRest): its
    far side, `thick` behind the front, the same outline seen from behind up to
    the cap line, in the picture `back` (-1: the wall's own below its top band),
    mirrored as seen from that side. `keep`: the stretch of the wall that
    stays (fractions from its a end), as the front's."""
    s, ua, va, ub, vb, z0, z1 = row[:7]
    zc, thick, back = _finite(row[9], "wall cap"), _finite(row[10], "wall thickness"), _int(row[11], "wall back")
    s = _int(s, "wall sprite index")
    if not 0 <= s < len(sprites) or not -1 <= back < len(sprites):
        raise ValueError("wall back sprite index is outside scene.sprites")
    _, front_x, front_y, w, front_h = sprites[s][:5]
    top = (z1 - zc) * hpx
    length = math.hypot(ub - ua, vb - va) or 1.0
    nu, nv = (vb - va) / length * thick, (ua - ub) / length * thick
    picture = back if back >= 0 else s
    x, y, h = (sprites[back][1], sprites[back][2], sprites[back][4]) if back >= 0 \
        else (front_x, front_y + top, front_h - top)
    if h <= 0 or zc <= z0:
        return
    # The back runs from b to a: its stretch is the front's, seen from there.
    part = (1.0 - min(keep[1], 1.0), 1.0 - max(keep[0], 0.0))
    _upright(batches, sprites, pages, picture, ub - nu, vb - nv, ua - nu, va - nv, z0, zc, x + w, y, -w, h, part)
    if _foot_z(row) is not None:
        r = _foot_row(atlas, sprites[picture][0], x, x + w, y + h)
        _upright(batches, sprites, pages, picture, ub - nu, vb - nv, ua - nu, va - nv, 0.0, z0,
                 x + w, r - 0.25, -w, 0.5, part)


def _wall_front(batches, proj, sprites, pages, row, toward_camera, continues, keep=(0.0, 1.0), top_keep=None,
                foot=None, atlas=None):
    """The front up to the cap line, then the slab's top as a real horizontal
    face as deep as the wall is thick, and what of the picture is higher than
    that top as an upright face on the slab's far side. Each point samples the
    texel the game drew it on, so without perspective the picture is the
    game's. Where the wall goes on in the same line (continues: a end, b end)
    the piece ends where its picture ends; a free end is cut square, as the
    slab is. `keep`: the stretch of the wall that stays (fractions from a to
    b), where a corner moved its end (_corner_fits); `top_keep` the top's
    (the front's when None). `foot`: the batches its foot goes in (_foot_z;
    None: none)."""
    top_keep = keep if top_keep is None else top_keep
    sprite, ua, va, ub, vb, z0, z1, _, _, zc, thick = row[:11]
    sprite = _int(sprite, "wall sprite")
    if not 0 <= sprite < len(sprites):
        raise ValueError(f"wall sprite index {sprite} is outside scene.sprites")
    page, x, y, w, h, *_ = sprites[sprite]
    page = _int(page, "wall sprite page")
    if not 0 <= page < len(pages):
        raise ValueError(f"wall sprite page {page} is outside scene.pages")
    x, y, w, h = (_int(value, "wall sprite rectangle") for value in (x, y, w, h))
    z0, z1, zc = (_finite(value, "wall height") for value in (z0, z1, zc))
    hpx = proj.hpx
    top = (z1 - zc) * hpx
    _upright(batches, sprites, pages, sprite, ua, va, ub, vb, z0, zc,
             x, y + top, w, h - top, (max(keep[0], 0.0), min(keep[1], 1.0)))
    if foot is not None and _foot_z(row) is not None:
        r = _foot_row(atlas, page, x, x + w, y + h)
        _upright(foot, sprites, pages, sprite, ua, va, ub, vb, 0.0, z0,
                 x, r - 0.25, w, 0.5, (max(keep[0], 0.0), min(keep[1], 1.0)))
    if top <= 0 or top_keep[1] <= top_keep[0]:
        return
    length = math.hypot(ub - ua, vb - va)
    e0, e1 = proj.ground_to_px(ua, va, zc), proj.ground_to_px(ub, vb, zc)
    dx = e1[0] - e0[0]
    if length == 0 or abs(dx) < 1e-9:
        raise ValueError("wall front has no width on the game's screen")
    t = ((ub - ua) / length, (vb - va) / length)
    back = (t[1], -t[0])
    if back[0] * toward_camera[0] + back[1] * toward_camera[1] > 0:
        back = (-back[0], -back[1])
    slope = (e1[1] - e0[1]) / dx

    def texel(point):
        """(column, px above the front's top line) the game drew `point` on."""
        qx, qy = proj.ground_to_px(*point)
        return (qx - e0[0]) * w / dx, e0[1] + slope * (qx - e0[0]) - qy

    def along(point):
        return (point[0] - ua) * t[0] + (point[1] - va) * t[1]

    def ends(polygon, square=(not continues[0], not continues[1])):
        # Every face ends where its picture does, so the pieces of a wall meet
        # without a gap; at a free end the top is also cut square, as the slab is.
        polygon = _clip(_clip(polygon, lambda p: texel(p)[0]), lambda p: w - texel(p)[0])
        if square[0]:
            polygon = _clip(polygon, along)
        return _clip(polygon, lambda p: length - along(p)) if square[1] else polygon

    reach = length + 4 * thick + 1
    at = lambda s, d, z: (ua + s * t[0] + d * back[0], va + s * t[1] + d * back[1], z)
    faces = [ends([at(-reach, 0, zc), at(reach, 0, zc), at(reach, thick, zc), at(-reach, thick, zc)])]
    rise = (top - texel(at(0, thick, zc))[1]) / hpx
    # Near a corner what the picture shows above the top is the other wall's
    # top, drawn in the same picture: the free camera has that wall itself.
    if rise > 1e-6 and keep == (0.0, 1.0) and top_keep == (0.0, 1.0):
        faces.append(ends([at(-reach, thick, zc), at(reach, thick, zc),
                           at(reach, thick, zc + rise), at(-reach, thick, zc + rise)], (False, False)))
    if top_keep != (0.0, 1.0):
        faces = [_clip(_clip(face, lambda p: along(p) - top_keep[0] * length),
                       lambda p: top_keep[1] * length - along(p)) for face in faces if len(face) >= 3]
    faces = [face for face in faces if len(face) >= 3]
    # A slab deeper than its picture's top rows takes the last row further.
    faces = _split(faces, lambda p: top - texel(p)[1])
    for face in faces:
        corners = []
        for point in face:
            column, rows = texel(point)
            column, rows = min(max(column, 0.0), w), min(max(rows, 0.0), top)
            # A texel keeps the depth of the front plane it was painted on: what
            # the game drew the wall over (a lamp post right behind it) stays hidden.
            f = column / w
            home = (ua + (ub - ua) * f - point[0], va + (vb - va) * f - point[1], zc + rows / hpx - point[2])
            corners.append([*point, x + column, y + top - rows, *home])
        _polygon(batches, proj, page, pages[page], corners)


def _continuations(walls):
    """For each thick wall row: (goes on at a, goes on at b), true where another
    thick wall of the same line shares that end."""
    near = 0.05
    cells = {}
    for index, row in enumerate(walls):
        if row[10] > 0:
            for u, v in ((row[1], row[2]), (row[3], row[4])):
                cells.setdefault((round(u / near), round(v / near)), []).append((index, u, v))
    out = {}
    for index, row in enumerate(walls):
        if not row[10] > 0:
            continue
        du, dv = row[3] - row[1], row[4] - row[2]
        flags = []
        for u, v in ((row[1], row[2]), (row[3], row[4])):
            cu, cv = round(u / near), round(v / near)
            found = False
            for key in ((cu + i, cv + j) for i in (-1, 0, 1) for j in (-1, 0, 1)):
                for other, ou, ov in cells.get(key, ()):
                    o = walls[other]
                    odu, odv = o[3] - o[1], o[4] - o[2]
                    if other != index and math.hypot(ou - u, ov - v) <= near and \
                            abs(du * odv - dv * odu) <= 0.02 * math.hypot(du, dv) * math.hypot(odu, odv):
                        found = True
            flags.append(found)
        out[index] = tuple(flags)
    return out


def _polygon(batches, proj, page, page_size, corners, away=False):
    """A convex polygon as a triangle fan, wound counter-clockwise on the
    screen like the wall fronts (they are drawn with back faces culled), or
    clockwise when it faces `away` from the game's camera: a solid's far side,
    culled by that camera and seen by a free one behind the thing."""
    area = screen_area(proj, corners)
    if abs(area) < 1e-6:
        return
    if (area < 0) != away:
        corners = corners[::-1]
    for index in range(1, len(corners) - 1):
        _quad_triangle(batches, page, page_size, (corners[0], corners[index], corners[index + 1]))


def _quad_triangle(batches, page, page_size, corners):
    width, height = page_size
    target = batches.setdefault(page, [])
    for corner in corners:
        u, v, z, tx, ty = (_finite(n, "solid position") for n in corner[:5])
        hu, hv, hz = (_finite(n, "solid depth") for n in corner[5:8]) if len(corner) > 5 else (0.0, 0.0, 0.0)
        if not (-1e-6 <= tx <= width + 1e-6 and -1e-6 <= ty <= height + 1e-6):
            raise ValueError("solid texture coordinates are outside the sprite atlas")
        target.extend((u - GL_MAP_OFFSET, z, v - GL_MAP_OFFSET,
                       min(max(tx / width, 0.0), 1.0), min(max(ty / height, 0.0), 1.0),
                       u + hu - GL_MAP_OFFSET, z + hz, v + hv - GL_MAP_OFFSET, batches.object))


# Corners (tasks 071, 076). The game draws a corner as one picture, and the
# donor lays each picture on its own wall's line: at a corner one wall runs on
# past the other by up to a hex and the other stops short of it, so the free
# camera sees a stub and a notch. Each end of a thick wall that meets another
# at a right angle is set where it belongs, face by face: at an L each wall's
# outer face runs to the other's outer face and its inner face to the other's
# inner face, so the outer faces meet in one edge and the inner ones in
# another, with no end face between; the tops stop at the inner faces and
# one cap covers the square where the walls cross. At a T the stem stops at
# the near face. An overrun is cut off for the free camera (the game's camera
# keeps the row whole), a shortfall is filled with the face's own material.
# The game's view stays as it was.
CORNER_REACH = 0.75  # how far an end may lie from the wall it meets
CORNER_SLACK = 0.02  # an end this close already meets it
CORNER_OVERLAP = 0.01  # added faces reach this far into their neighbours: no crack shows between them


class _Slab:
    """Thick wall rows in one line: `axis` 0 runs along u (v = plane), 1 along
    v (u = plane); the slab lies between its front `plane` and `side` times
    `thick` from it, away from the game's camera; it spans lo .. hi."""

    def __init__(self, axis, plane, side):
        self.axis, self.plane, self.side = axis, plane, side
        self.thick, self.rows, self.lo, self.hi = 0.0, [], math.inf, -math.inf

    def band(self):
        return min(self.plane, self.plane + self.side * self.thick), max(self.plane, self.plane + self.side * self.thick)

    def point(self, s, depth):
        return (s, self.plane + self.side * depth) if self.axis == 0 else (self.plane + self.side * depth, s)


def _slabs(walls, hidden, toward_camera):
    lines = {}
    for index, row in enumerate(walls):
        if not row[10] > 0 or index in hidden:
            continue
        ua, va, ub, vb = row[1:5]
        if abs(va - vb) < 1e-6:
            lines.setdefault((0, round(va, 3)), []).append((min(ua, ub), max(ua, ub), index))
        elif abs(ua - ub) < 1e-6:
            lines.setdefault((1, round(ua, 3)), []).append((min(va, vb), max(va, vb), index))
    slabs = []
    for (axis, plane), pieces in sorted(lines.items()):
        slab = None
        for lo, hi, index in sorted(pieces):
            if slab is None or lo > slab.hi + 0.05:
                slab = _Slab(axis, plane, -1.0 if toward_camera[1 - axis] > 0 else 1.0)
                slabs.append(slab)
            slab.lo, slab.hi = min(slab.lo, lo), max(slab.hi, hi)
            slab.thick = max(slab.thick, walls[index][10])
            slab.rows.append(index)
    return slabs


class _Corners:
    """What the corner pass changes for the free camera. keep, back_keep,
    top_keep {wall row: (from, to)}: the stretch of the row's front, back and
    top it draws, as fractions from the row's a end (the top may run beyond 0
    or 1); fills [(slab, rows, s from, s to, back)]: a shortfall of one face
    (the back when `back`) in the most typical columns of that face of
    `rows`; caps [(slab, row, s from, s to)]: a flat piece of top across the
    slab; ends [(slab, outward, s, rows)]: a wall's end face where it
    meets a thin wall, in the most typical columns of `rows`;
    moved [(slab, old end)]: the ends whose donor end strips no longer stand at
    the wall's end; cores [(slab, other, row)]: the corner square's solid
    core (walls at least CORE_THICK thick)."""

    def __init__(self):
        self.keep, self.back_keep, self.top_keep = {}, {}, {}
        self.fills, self.caps, self.moved, self.cores, self.ends = [], [], [], [], []
        self.extent = {}  # {id(slab): (from, to)} what the free camera keeps of a slab a corner moved
        self.pieces = {}  # {wall row: (slab, out, twin, twin's out, start)} the corner pieces left out
        self.pools = {}  # {(id(slab), out): the plain rows near that end}


def _corner_fits(walls, hidden, toward_camera, width=None, px=(1.0, 1.0)):
    """width(row): its picture's columns; px: picture columns per unit along
    u and along v. A block, face or cap takes its picture from a row at least
    half as wide as the columns it needs (task 076): the narrow end strips
    hold only the light edge, and stretched over a block they read as
    stripes."""
    slabs = _slabs(walls, hidden, toward_camera)
    wide = lambda index, need: width is None or width(index) >= need
    gap = lambda x, lo, hi: max(lo - x, 0.0, x - hi)
    far = lambda slab, out: slab.band()[1] if out > 0 else slab.band()[0]
    near = lambda slab, out: slab.band()[0] if out > 0 else slab.band()[1]

    def tip(slab, out, other):
        """Where the slab's end at `out` lies for a corner with `other`. A
        corner picture may run on past the crossing out of reach (a post or
        a trim standing out in the open, task 081): a last row that runs
        across the other's band and stops within twice the reach past it
        ends the slab in the crossing."""
        end = slab.hi if out > 0 else slab.lo
        over = out * (end - far(other, out))
        if not CORNER_REACH < over <= 2 * CORNER_REACH:
            return end
        for index in slab.rows:
            s0, s1 = sorted((walls[index][1 + slab.axis], walls[index][3 + slab.axis]))
            last, first = (s1, s0) if out > 0 else (-s0, -s1)
            if last >= out * end - 1e-6 and first < out * far(other, out) - CORNER_SLACK:
                return far(other, out)
        return end

    targets = {}
    pairs = {}  # an L: {(id(slab), out): (the other slab, its out)}
    meets = {}  # {(id(slab), out): the slab that end meets}
    for a in slabs:
        a0, a1 = a.band()
        for end, out in ((a.lo, -1), (a.hi, 1)):
            if (id(a), out) in targets:
                continue
            # A short wall may come within reach of a wall with both ends: a
            # wall that reaches past its other end is that end's corner.
            past = lambda b: b.band()[0] <= a.lo + CORNER_SLACK if out > 0 else b.band()[1] >= a.hi - CORNER_SLACK
            met = [(gap(tip(a, out, b), *b.band()), index, b) for index, b in enumerate(slabs) if b.axis != a.axis
                   and gap(tip(a, out, b), *b.band()) <= CORNER_REACH
                   and b.hi >= a0 - CORNER_REACH and b.lo <= a1 + CORNER_REACH
                   and not (past(b) and gap(tip(a, out, b), *b.band()) > CORNER_SLACK)
                   # A thick wall does not reach for a rail or a trim strip
                   # (some are wholly clear): the game ends it where it ends.
                   and not (a.thick >= CORE_THICK and b.thick < CORE_THICK)]
            if not met:
                continue
            b = min(met, key=lambda item: item[:2])[2]
            end = tip(a, out, b)
            b_lo, b_hi = tip(b, -1, a), tip(b, 1, a)
            if b_lo < a0 - CORNER_REACH and b_hi > a1 + CORNER_REACH:
                targets[(id(a), out)] = (a, out, near(b, out), None)  # a T: the stem stops at b
                meets[(id(a), out)] = b
                continue
            if b_hi <= a1 + CORNER_REACH and b_lo >= a0 - CORNER_REACH:
                continue  # all of b lies at a's end: no corner to read
            b_out = 1 if b_hi <= a1 + CORNER_REACH else -1
            b_end = b_hi if b_out > 0 else b_lo
            # The wall that runs on further past the other reaches across it;
            # its end face then lies in the other's plane.
            if out * (end - far(b, out)) >= b_out * (b_end - far(a, b_out)):
                targets[(id(a), out)] = (a, out, far(b, out), b)
                meets[(id(a), out)] = b
                if not (b.thick >= CORE_THICK and a.thick < CORE_THICK):
                    targets.setdefault((id(b), b_out), (b, b_out, near(a, b_out), None))
                    meets.setdefault((id(b), b_out), a)
            else:
                targets[(id(a), out)] = (a, out, near(b, out), None)
                meets[(id(a), out)] = b
                if not (b.thick >= CORE_THICK and a.thick < CORE_THICK):
                    targets.setdefault((id(b), b_out), (b, b_out, far(a, b_out), a))
                    meets.setdefault((id(b), b_out), a)
            # A rail or a trim strip is no wall to turn a corner with.
            if (id(b), b_out) in targets and targets[(id(b), b_out)][2] in (near(a, b_out), far(a, b_out)) \
                    and (id(b), b_out) not in pairs \
                    and min(a.thick, b.thick) >= CORE_THICK:
                pairs[(id(a), out)], pairs[(id(b), b_out)] = (b, b_out), (a, out)
    corners = _Corners()
    corners.targets = list(targets.values())
    pieces = set()  # the corner pieces left out
    # A rail or trim strip that lies wholly within reach of a thick wall's
    # line is the corner's trim (a ledge standing up out of the roof): the
    # free camera leaves it out.
    for a in slabs:
        if a.thick >= CORE_THICK:
            continue
        for b in slabs:
            if b.axis != a.axis and b.thick >= CORE_THICK and max(gap(a.lo, *b.band()), gap(a.hi, *b.band())) \
                    <= CORNER_REACH and b.lo - CORNER_SLACK <= a.plane <= b.hi + CORNER_SLACK:
                for index in a.rows:
                    corners.keep[index] = corners.back_keep[index] = corners.top_keep[index] = (1.0, 1.0)
                corners.moved += [(a, a.lo), (a, a.hi)]
                break
    for slab, out, target, other in targets.values():
        end = slab.hi if out > 0 else slab.lo
        axis = slab.axis
        s_of = lambda row, at: row[1 + 2 * at + axis]  # the row's s at its a (0) or b (1) end
        span = lambda index: sorted((s_of(walls[index], 0), s_of(walls[index], 1)))
        reach = lambda index: span(index)[1] if out > 0 else -span(index)[0]
        # Each face's own end. At an L the face on the corner's outside (away
        # from the other wall's body) runs to the other's outer face, the
        # inside one to its inner face; the top stops at the inner face.
        front_t = back_t = top_t = target
        if (id(slab), out) in pairs:
            twin, twin_out = pairs[(id(slab), out)]
            front_outer = -slab.side * twin_out > 0  # the front faces away from the slab's body
            outer, inner = far(twin, out), near(twin, out)
            front_t, back_t, top_t = (outer, inner, inner) if front_outer else (inner, outer, inner)
        near_end = [index for index in slab.rows if out * (end - (span(index)[1] if out > 0 else span(index)[0]))
                    <= 2 * CORNER_REACH]
        # Up to twice stretched reads as the wall; a nearer, wider row would
        # repeat what stands further in (a window). A cap's colour comes from it.
        need = max(abs(end - target) * px[axis] / 2, 4)
        fits = [index for index in near_end if wide(index, need)]
        lender = max(fits, key=reach) if fits else \
            max(near_end or slab.rows, key=lambda index: (width(index) if width else 0, reach(index)))
        # At an L the corner piece, a row that runs into the square where the
        # walls cross, is left out for the free camera: the game draws the
        # corner in it seen at an angle, both walls in one picture, and on one
        # wall's plane the other's bricks come out skewed. Its faces are
        # filled in their own wall's plain material from the last plain row,
        # so the bands of the two walls meet in the corner's edge.
        o0, o1 = pairs[(id(slab), out)][0].band() if (id(slab), out) in pairs else (math.inf, -math.inf)
        clear = lambda index: max(span(index)) <= o0 + CORNER_SLACK or min(span(index)) >= o1 - CORNER_SLACK
        plain = [index for index in near_end if clear(index) and out * ((span(index)[1] if out > 0 else span(index)[0])
                                                                        - (o0 if out > 0 else o1)) <= CORNER_SLACK]
        start = end
        # A short wall keeps its pieces: both its ends may be corners, and
        # each would leave out the other's plain row.
        if (id(slab), out) in pairs and plain and any(not clear(index) for index in near_end) \
                and slab.hi - slab.lo > 2 * CORNER_REACH:
            start = max((span(index)[1] if out > 0 else span(index)[0] for index in plain), key=lambda t: out * t)
        elif (id(slab), out) in pairs and not any(clear(index) for index in slab.rows) \
                and slab.hi - slab.lo <= 2 * CORNER_REACH:
            # A wall that is nothing but a corner piece is left out whole;
            # its faces are filled from its far end in its line's material.
            start = slab.lo if out > 0 else slab.hi
        left_out = set()
        # Every row near this end stops at its faces' ends, its top included: a
        # top runs on past its picture's edge and would stick out of the corner.
        for index in slab.rows:
            lo_s, hi_s = span(index)
            if out * ((hi_s if out > 0 else lo_s) - min(front_t, back_t, key=lambda t: out * t)) < -2 * CORNER_REACH:
                continue
            row = walls[index]
            sa, sb = s_of(row, 0), s_of(row, 1)
            if sb == sa:
                continue
            for store, at in ((corners.keep, front_t), (corners.back_keep, back_t), (corners.top_keep, top_t)):
                if start != end and index not in plain and index in near_end and not clear(index):
                    store[index] = (1.0, 1.0)  # the corner piece, whole: the plain rows cover its overlap
                    pieces.add(index)
                    left_out.add(index)
                    continue
                cut = (at - sa) / (sb - sa)
                lo, hi = store.get(index, (-math.inf, math.inf))
                if (sb - sa) * out > 0:
                    hi = min(hi, cut)
                else:
                    lo = max(lo, cut)
                store[index] = (lo, hi)
        # A face that falls short takes its own material up to its end: the
        # rows of this line near the corner (past a gap too), off the square;
        # the nearest first, whose height the face takes.
        pool = sorted([index for line in slabs if line.axis == slab.axis and abs(line.plane - slab.plane) < 0.01
                       for index in line.rows
                       if (max(span(index)) <= o0 + CORNER_SLACK or min(span(index)) >= o1 - CORNER_SLACK)
                       and gap(end, *span(index)) <= 2 * CORNER_REACH and wide(index, 4)
                       # a strip high up (a top band of its own) is no face to fill with
                       and walls[index][5] <= FOOT_MAX],
                      key=lambda index: gap(start, *span(index))) or [lender]
        corners.pools[(id(slab), out)] = pool
        for index in left_out:
            corners.pieces[index] = (slab, out, *pairs[(id(slab), out)], start)
        for at, back in ((front_t, False), (back_t, True)):
            if out * (start - at) < -CORNER_SLACK:
                corners.fills.append((slab, pool, start, at, back))
        if max(abs(end - front_t), abs(end - back_t), abs(end - start)) > CORNER_SLACK:
            corners.moved.append((slab, end))
            # Its donor end strip is gone. At an L the other wall's faces close
            # the end; a rail or a trim strip does not, nor a wall that does
            # not turn the corner back (a short one, all of it at this end):
            # then it gets an end of its own.
            met = meets.get((id(slab), out))
            if met is not None and (id(slab), out) not in pairs and (met.thick < CORE_THICK or other is not None):
                corners.ends.append((slab, out, target, pool))
        if other is not None and (id(slab), out) in pairs and min(slab.thick, other.thick) >= CORE_THICK:
            corners.cores.append((slab, other, lender))
        # The cap stays within what is left of the slab: a short corner piece
        # may have both of its ends set.
        other_end = targets.get((id(slab), -out))
        stop = other_end[2] if other_end else (slab.lo if out > 0 else slab.hi)
        reach_to = max(front_t, back_t, key=lambda t: out * t)
        corners.extent[id(slab)] = (stop, reach_to) if out > 0 else (reach_to, stop)
        reach = min(top_t - out * max(2 * slab.thick, 0.35), start, key=lambda t: out * t)
        corners.caps.append((slab, lender, max(reach, stop) if out > 0 else min(reach, stop), top_t))
        if other is not None and (id(slab), out) in pairs:
            # The square where the two walls cross: one cap over it.
            twin = pairs[(id(slab), out)][0]
            corners.caps.append((slab, lender, near(twin, out), far(twin, out)))
    # No face takes its material from a corner piece.
    plain_pool = lambda pool: [index for index in pool if index not in pieces] or pool
    corners.fills = [(slab, plain_pool(pool), s0, s1, back) for slab, pool, s0, s1, back in corners.fills]
    corners.ends = [(slab, out, at, plain_pool(pool)) for slab, out, at, pool in corners.ends]
    corners.pools = {key: plain_pool(pool) for key, pool in corners.pools.items()}
    return corners


def _picture(sprites, row, hpx):
    """(page, x, y, w, h, rows of its top band) of a wall row's picture."""
    page, x, y, w, h = (_int(value, "wall sprite rectangle") for value in sprites[_int(row[0], "wall sprite")][:5])
    return page, x, y, w, h, min(max((row[6] - row[9]) * hpx, 0.0), h - 1.0)


def _strip(row, axis, at, px, w, x, n):
    """Columns (from, to) of the n at the picture's edge nearest s = at; the
    row's a end shows column x."""
    sa, sb = row[1 + axis], row[3 + axis]
    n = min(max(round(n * px), 1), w)
    return (x, x + n) if abs(sa - at) < abs(sb - at) else (x + w, x + w - n)


def _face_picture(sprites, row, hpx, back):
    """(page, x, y, w, h) of what a wall row shows below its top band: on its
    front, or on its back (its `back` picture, else the front's)."""
    page, x, y, w, h, top = _picture(sprites, row, hpx)
    other = _int(row[11], "wall back")
    if back and other >= 0:
        return tuple(_int(value, "wall sprite rectangle") for value in sprites[other][:5])
    return page, x, y + top, w, h - top


def _typical_columns(atlas, sprites, hpx, walls, pool, back, n):
    """(page, first column, last column, top row, bottom row, row) of the n
    picture columns of one face of the rows in `pool` whose brightness is
    nearest the median column of them all: the wall's plain stretch, not a
    window frame or a light post (task 076). Without the atlas, the middle
    of the first row's."""
    pictures = {index: _face_picture(sprites, walls[index], hpx, back) for index in pool}
    if atlas is None:
        page, x, y, w, h = pictures[pool[0]]
        k = min(n, w)
        return page, x + (w - k) / 2, x + (w + k) / 2, y, y + h, pool[0]
    shade, solid = {}, {}
    for index, (page, x, y, w, h) in pictures.items():
        block = atlas[page][int(y):int(y + h), x:x + w].astype(float)
        shade[index] = (block[:, :, :3].sum(axis=2) * (block[:, :, 3] >= 128)).mean(axis=0)
        solid[index] = (block[:, :, 3] >= 128).mean(axis=0)
    values = sorted(v for columns in shade.values() for v in columns)
    target = values[len(values) // 2]
    best = None
    for index in pool:
        columns = shade[index]
        k = max(min(round(n), len(columns)), 1)
        for j in range(len(columns) - k + 1):
            # Clear texels would open a hole in the face: opaque columns first.
            miss = abs(sum(columns[j:j + k]) / k - target) + 1000.0 * (1.0 - min(solid[index][j:j + k]))
            if best is None or miss < best[0] - 1e-9:
                best = (miss, index, j, k)
    _, index, j, k = best
    page, x, y, w, h = pictures[index]
    return page, x + j, x + j + k, y, y + h, index


FILL_SEAM = 0.02
FILL_RECESS = 0.002
FILL_EDGE = 0.003


REPEAT_COLUMNS = (8, 16)  # a front up to a corner's decor repeats this many of its last plain columns


def _corner_fill(batches, proj, sprites, pages, hpx, px, walls, pool, slab, s0, s1, back, atlas=None, repeat=False):
    """One face of a block of the wall between s0 and s1 along `slab`: its
    front, or its back when `back`, in the most typical columns of that face
    of the rows in `pool` (_typical_columns). px: picture columns per unit
    along u and along v. `repeat` (a front up to a corner's decor, task
    083): the last REPEAT_COLUMNS columns of the nearest row wide enough,
    over and over, so the wall runs on as it stands up to the decor."""
    # It runs FILL_SEAM under the row it continues, a hair deeper than that
    # row's face, so the row hides the overlap (no flickering strip) and no
    # crack opens between them; at s1 it reaches FILL_EDGE past the corner's
    # edge, so the two walls' faces meet there without a crack.
    sign = 1.0 if s1 > s0 else -1.0
    start, stop = s0 - sign * FILL_SEAM, s1 + sign * FILL_EDGE
    p = slab.point
    thick = walls[pool[0]][10]  # the row it continues: its back is this deep
    depth = thick - FILL_RECESS if back else FILL_RECESS
    wide = [index for index in pool if _face_picture(sprites, walls[index], hpx, False)[3] >= REPEAT_COLUMNS[0]]
    if repeat and wide and not back:
        lender = wide[0]
        page, x, y_top, w, h = _face_picture(sprites, walls[lender], hpx, False)
        y_bottom = y_top + h
        # Columns from the one a span further in to the edge nearest the corner.
        edge, inner = _strip(walls[lender], slab.axis, s0, 1.0, w, x, min(REPEAT_COLUMNS[1], w))
        span = abs(edge - inner) / px[slab.axis]
        length = abs(stop - start)
        tiles = [(start + sign * span * n, start + sign * min(length, span * (n + 1)))
                 for n in range(math.ceil(length / span))]
    else:
        page, c0, c1, y_top, y_bottom, lender = _typical_columns(atlas, sprites, hpx, walls, pool, back,
                                                         abs(s1 - s0) * px[slab.axis])
        tiles = None
    z0, zc = walls[lender][5], walls[lender][9]  # as high as the row the columns come from
    foot = _foot_z(walls[lender])

    def block(a, b, u0, u1):
        corners = [[*p(a, depth), z0, u0, y_bottom], [*p(b, depth), z0, u1, y_bottom],
                   [*p(b, depth), zc, u1, y_top], [*p(a, depth), zc, u0, y_top]]
        _polygon(batches, proj, page, pages[page], corners, back)
        if foot is not None:  # down to the floor, as the row's own foot
            r = _foot_row(atlas, page, u0, u1, y_bottom)
            corners = [[*p(a, depth), 0.0, u0, r + 0.25], [*p(b, depth), 0.0, u1, r + 0.25],
                       [*p(b, depth), z0, u1, r - 0.25], [*p(a, depth), z0, u0, r - 0.25]]
            _polygon(batches, proj, page, pages[page], corners, back)

    if tiles is None:
        block(*sorted((start, stop)), c0, c1)
        return
    for a, b in tiles:
        block(a, b, inner, inner + (edge - inner) * abs(b - a) / span)


def _corner_end(batches, proj, sprites, pages, hpx, px, toward_camera, walls, pool, slab, out, s, atlas=None):
    """The end face of `slab` at s across its thickness, in the most typical
    columns of its rows' fronts (_typical_columns)."""
    page, c0, c1, y_top, y_bottom, lender = _typical_columns(atlas, sprites, hpx, walls, pool, False,
                                                     slab.thick * px[1 - slab.axis])
    z0, zc = walls[lender][5], walls[lender][9]  # as high as the row the columns come from
    p = slab.point
    near, far = -CORNER_OVERLAP, slab.thick + CORNER_OVERLAP
    corners = [[*p(s, near), z0, c0, y_bottom], [*p(s, far), z0, c1, y_bottom],
               [*p(s, far), zc, c1, y_top], [*p(s, near), zc, c0, y_top]]
    _polygon(batches, proj, page, pages[page], corners, out * toward_camera[slab.axis] < 0)
    if _foot_z(walls[lender]) is not None:
        r = _foot_row(atlas, page, c0, c1, y_bottom)
        corners = [[*p(s, near), 0.0, c0, r + 0.25], [*p(s, far), 0.0, c1, r + 0.25],
                   [*p(s, far), z0, c1, r - 0.25], [*p(s, near), z0, c0, r - 0.25]]
        _polygon(batches, proj, page, pages[page], corners, out * toward_camera[slab.axis] < 0)


def _corner_cap(batches, proj, sprites, pages, hpx, row, slab, s0, s1):
    """The wall's top over s0 .. s1 as one flat piece, a hair above the tops it
    covers: the sheared tops end in a slant at a corner and leave a notch
    there. Its colour is the texel the game drew the top on at its middle,
    as the wall's own top samples it (_wall_front)."""
    page, x, y, w, h, top = _picture(sprites, row, hpx)
    if top < 1:
        return
    ua, va, ub, vb, zc = row[1], row[2], row[3], row[4], row[9]
    e0, e1 = proj.ground_to_px(ua, va, zc), proj.ground_to_px(ub, vb, zc)
    dx = e1[0] - e0[0]
    if abs(dx) < 1e-9:
        return
    middle = slab.point((s0 + s1) / 2, slab.thick / 2)
    qx, qy = proj.ground_to_px(*middle, zc)
    column = min(max((qx - e0[0]) * w / dx, 0.5), w - 0.5)
    rows = min(max(e0[1] + (e1[1] - e0[1]) / dx * (qx - e0[0]) - qy, 0.5), top - 0.5)
    tx, ty = x + column, y + top - rows
    z = zc + 0.004
    p = slab.point
    lo, hi = sorted((s0, s1))
    _polygon(batches, proj, page, pages[page], [[*p(lo, 0.0), z, tx, ty], [*p(hi, 0.0), z, tx, ty],
                                                [*p(hi, slab.thick), z, tx, ty], [*p(lo, slab.thick), z, tx, ty]])


DECOR_LIFT = 0.004  # a folded corner picture stands this far out of the faces it lies on, over their fill


def _front_end(slab, out, twin, twin_out):
    """Where the front of `slab` ends at its corner with `twin` (_corner_fits):
    on the corner's outside at the other's outer face, else at its inner one."""
    outer = -slab.side * twin_out > 0
    band = twin.band()
    return (band[1] if out > 0 else band[0]) if outer else (band[0] if out > 0 else band[1])


def _corner_decor(batches, proj, sprites, pages, toward_camera, row, slab, out, twin, twin_out, lift=DECOR_LIFT):
    """A corner piece with decor (task 083), folded onto the two walls'
    fronts: each column of its picture lies on the front the game's camera
    sees at that point of the screen (the nearer of the two where both
    stand), a hair out of it, so the decor sits on the corner. Each point
    samples the texel the game drew it on: from the game's camera it is the
    picture pixel for pixel. Below the cap line only: the caps cover the
    tops."""
    page, x, y, w, h = (_int(value, "wall sprite rectangle") for value in sprites[_int(row[0], "wall sprite")][:5])
    ua, va, ub, vb, z0, zc = row[1], row[2], row[3], row[4], row[5], row[9]
    e0, e1 = proj.ground_to_px(ua, va, z0), proj.ground_to_px(ub, vb, z0)
    dx = e1[0] - e0[0]
    if abs(dx) < 1e-9 or zc <= z0:
        return
    slope = (e1[1] - e0[1]) / dx

    def texel(point):
        """(column, px above the picture's bottom line) the game drew `point` on."""
        qx, qy = proj.ground_to_px(*point)
        return (qx - e0[0]) * w / dx, e0[1] + slope * (qx - e0[0]) - qy

    fronts = []
    for wall, wall_out, other, other_out in ((slab, out, twin, twin_out), (twin, twin_out, slab, out)):
        x0 = proj.ground_to_px(*wall.point(0.0, -lift), 0.0)[0]
        rate = proj.ground_to_px(*wall.point(1.0, -lift), 0.0)[0] - x0
        if abs(rate) > 1e-9:
            # s on this front where the screen shows column k; the front runs
            # from its own wall up to its end at the corner
            at = lambda k, wall=wall, x0=x0, rate=rate: (e0[0] + dx * k / w - x0) / rate
            end = _front_end(wall, wall_out, other, other_out)
            if abs(end - other.plane) < 1e-9:
                end -= other.side * lift  # the fronts meet out of the faces as far as they stand
            fronts.append((wall, wall_out, end, at))
    # Exactly to its end: past it the other front shows (the nearer of two
    # standing fronts is the right one only where both truly stand).
    stands = lambda front, k: front[1] * (front[3](k) - front[2]) <= 1e-9
    nearness = lambda front, k: sum(p * t for p, t in zip(front[0].point(front[3](k), -lift), toward_camera))
    # Columns where a front ends or the two cross split the picture.
    cuts = {0.0, float(w)}
    for front in fronts:
        f0, f1 = front[3](0.0), front[3](1.0)
        if abs(f1 - f0) > 1e-12:
            cuts.add((front[2] - f0) / (f1 - f0))
    if len(fronts) == 2:
        d0, d1 = (nearness(fronts[0], k) - nearness(fronts[1], k) for k in (0.0, 1.0))
        if abs(d1 - d0) > 1e-12:
            cuts.add(d0 / (d0 - d1))
    cuts = sorted(k for k in cuts if 0.0 <= k <= w)
    for k0, k1 in zip(cuts, cuts[1:]):
        if k1 - k0 < 1e-6:
            continue
        mid = (k0 + k1) / 2
        seen = [front for front in fronts if stands(front, mid)]
        if not seen:
            continue  # neither front stands there: open air
        wall, _, _, at = max(seen, key=lambda front: nearness(front, mid))
        p, q = wall.point(at(k0), -lift), wall.point(at(k1), -lift)
        face = [(*p, z0), (*q, z0), (*q, zc), (*p, zc)]
        for side in (lambda t: texel(t)[1], lambda t: h - texel(t)[1]):
            face = _clip(face, side)
        if len(face) < 3:
            continue
        corners = []
        for point in face:
            column, rows = texel(point)
            corners.append([*point, x + min(max(column, 0.0), w), y + h - min(max(rows, 0.0), h)])
        _polygon(batches, proj, page, pages[page], corners)


DECOR_DULL = 12.0  # a corner piece more colourful than this (mean channel spread) is brick or boards
DECOR_SHADE = 8.0  # a corner piece this much darker or lighter than the plain wall carries decor
DECOR_PATTERN = 0.3  # ... or this share of its columns matches no plain column (_odd_columns)
DECOR_SHIFT = 12  # rows a plain column may slide up or down to match


def _gray(block):
    """Brightness of each texel of an RGBA block, NaN where it is clear."""
    import numpy as np
    return np.where(block[:, :, 3] >= 128, block[:, :, :3].sum(axis=2) / 3, np.nan)


def _column_misses(plain, columns, rows):
    """For each of `columns` (rows x n, NaN clear), its least mean brightness
    gap to a column of `plain`, slid up to DECOR_SHIFT rows either way; a
    clear texel against an opaque one counts as the widest gap. `rows`
    picks the plain columns each may not match (None: any)."""
    import numpy as np
    height = min(plain.shape[0], columns.shape[0]) - 2 * DECOR_SHIFT
    if height < 4:
        return None
    q = columns[DECOR_SHIFT:DECOR_SHIFT + height, None, :]
    best = np.full(columns.shape[1], np.inf)
    for shift in range(2 * DECOR_SHIFT + 1):
        seg = plain[shift:shift + height, :, None]
        gap = np.abs(seg - q)
        gap = np.where(np.isnan(seg) & np.isnan(q), 0.0, np.where(np.isnan(gap), 255.0, gap)).mean(axis=0)
        if rows is not None:
            gap[rows] = np.inf
        best = np.minimum(best, gap.min(axis=0))
    return best


def _odd_columns(plain, piece):
    """Share of the piece's columns that match no column of the plain wall
    within twice as far as the plain wall's columns match each other."""
    import numpy as np
    picks = np.arange(0, plain.shape[1], 3)
    near = np.abs(np.arange(plain.shape[1])[:, None] - picks[None, :]) <= 2
    spread = _column_misses(plain, plain[:, picks], near)
    misses = _column_misses(plain, piece, None)
    if spread is None or misses is None:
        return 0.0
    return float(np.mean(misses > 2 * np.median(spread)))


def _decor_pieces(atlas, sprites, hpx, walls, corners):
    """The corner pieces whose picture carries decor (task 083): a dull piece
    darker or lighter than the plain rows near its end, or with columns
    unlike any of theirs. A colourful piece is brick or boards, the wall's
    own material, and keeps the plain fill. Without the atlas, none."""
    if atlas is None:
        return []
    import numpy as np

    def block(index):
        page, x, y, w, h = _face_picture(sprites, walls[index], hpx, False)
        return atlas[page][int(y):int(y + h), x:x + w].astype(float)

    found = []
    for index, (slab, out, *_) in sorted(corners.pieces.items()):
        piece = block(index)
        plains = [b for b in map(block, corners.pools.get((id(slab), out), ())) if b.shape[1] >= 4]
        if piece.shape[1] < 4 or not plains:
            continue
        opaque = piece[piece[:, :, 3] >= 128][:, :3]
        if not len(opaque) or np.mean(opaque.max(axis=1) - opaque.min(axis=1)) > DECOR_DULL:
            continue
        height = min(b.shape[0] for b in plains)
        plain = np.concatenate([b[:height] for b in plains], axis=1)
        shade = opaque.mean() - plain[plain[:, :, 3] >= 128][:, :3].mean()
        if abs(shade) >= DECOR_SHADE or _odd_columns(_gray(plain), _gray(piece)) >= DECOR_PATTERN:
            found.append(index)
    return found


def _opaque_texel(atlas, page, x, y, w, h):
    """An opaque texel of a picture rectangle of its median brightness: the
    wall's everyday colour, not a highlight (the middle without the atlas)."""
    cx, cy = x + w / 2, y + h / 2
    if atlas is None:
        return cx, cy
    block = atlas[page][int(y):int(y + h), int(x):int(x + w)]
    found = sorted((int(block[i, j, 0]) + int(block[i, j, 1]) + int(block[i, j, 2]), i, j)
                   for i in range(block.shape[0]) for j in range(block.shape[1]) if block[i, j, 3] >= 128)
    if not found:
        return cx, cy
    _, i, j = found[len(found) // 2]
    return int(x) + j + 0.5, int(y) + i + 0.5  # the block starts at the whole pixel


CORE_SOLID = 0.9  # a wall whose front pictures are this opaque is a solid wall, not a mesh


def _solidity(walls, sprites, hpx, atlas, slab):
    """The median share of opaque texels on the fronts of a slab's rows (the
    narrow end strips left out); 1 without the atlas."""
    if atlas is None:
        return 1.0
    shares = []
    for index in slab.rows:
        page, x, y, w, h, top = _picture(sprites, walls[index], hpx)
        if w >= 8 and h - top >= 1:
            shares.append(float((atlas[page][int(y + top):int(y + h), x:x + w, 3] >= 128).mean()))
    shares.sort()
    return shares[len(shares) // 2] if shares else 0.0


def _corner_core(batches, proj, sprites, pages, hpx, toward_camera, row, slab, other, extent, solid, cap,
                 atlas=None):
    """An opaque box just inside the square where two walls of a corner
    overlap. A corner piece's picture is often clear along its edge, where
    the game shows the other wall's picture behind it; the free camera would
    look through there into the hollow wall. Seen only through such texels,
    it is one opaque colour of the wall's front. Each arm stays below its
    own wall's top (cap(slab)): a lower wall gets no fin."""
    # A mesh fence is clear on purpose: a corner gets a core only where a
    # solid wall stands, and it runs on only into solid walls (solid(slab)).
    if max(solid(slab), solid(other)) < CORE_SOLID or min(solid(slab), solid(other)) < CORE_SOLID / 2:
        return
    page, x, y, w, h, top = _picture(sprites, row, hpx)
    z0 = row[5]
    tx, ty = _opaque_texel(atlas, page, x, y + top, w, h - top)
    # The corner square and CORE_REACH of each wall beyond it: the clear
    # edge of a corner piece runs on past the square. Within what the free
    # camera keeps of each wall (`extent`), so it never stands in the open.
    middle = lambda b: sum(b.band()) / 2
    for a, b in ((slab, other), (other, slab)):
        lo, hi = extent.get(id(a), (a.lo, a.hi))
        inward = -1 if abs(hi - middle(b)) < abs(lo - middle(b)) else 1  # from the corner into a
        inner = b.band()[1 if inward > 0 else 0] + inward * (CORE_REACH if solid(a) >= CORE_SOLID else 0.0)
        inner = min(inner, hi) if inward > 0 else max(inner, lo)
        along = sorted((b.band()[0 if inward > 0 else 1], inner))
        _box(batches, proj, pages, toward_camera, a.axis, along, a.band(), z0, cap(a) - 0.006, page, tx, ty)


CORE_REACH = 0.5  # how far a corner's core runs into each wall past the square
CORE_THICK = 0.15  # thinner walls (fences, rails) get no core


def _box(batches, proj, pages, toward_camera, axis, along, across, z0, z1, page, tx, ty):
    """The four sides of a box one colour, a hair inside the given ranges:
    `along` the axis, `across` it."""
    inset = 0.006
    (a0, a1), (b0, b1) = along, across
    a0, a1, b0, b1 = a0 + inset, a1 - inset, b0 + inset, b1 - inset
    if a1 <= a0 or b1 <= b0 or z1 <= z0:
        return
    point = lambda a, b: (a, b) if axis == 0 else (b, a)
    ring = [point(a0, b0), point(a1, b0), point(a1, b1), point(a0, b1)]
    cu, cv = point((a0 + a1) / 2, (b0 + b1) / 2)
    for index, p in enumerate(ring):
        q = ring[(index + 1) % 4]
        # A side facing away from the game's camera is wound for the far side.
        outward = ((p[0] + q[0]) / 2 - cu) * toward_camera[0] + ((p[1] + q[1]) / 2 - cv) * toward_camera[1]
        corners = [[*p, z0, tx, ty], [*q, z0, tx, ty], [*q, z1, tx, ty], [*p, z1, tx, ty]]
        _polygon(batches, proj, page, pages[page], corners, outward < 0)


# Roofs (task 081). The engine lays each roof square flat at the roof
# height, with its cell's rim past each side, and the game draws roofs over
# the walls. What a wall's picture shows above that height under a roof (an
# inner wall's rise face, a band of top trim) is hidden in the game's view,
# but stuck out of the roof for the free camera. The free camera keeps each
# wall face only below the roof there; the game's camera keeps it whole.
ROOF_SLACK = 0.001  # a face this little over the roof is under it (the corner caps lie a hair under)
ROOF_CELL, ROOF_MARGIN = 88, 6  # a roof square's texels and its rim's (floor_sidecar, floor.cc)
ROOF_RIM = ROOF_MARGIN / ROOF_CELL  # how far past its square the engine draws a roof cell's rim


class _Roofs:
    """The roof squares of a scene at the roof height. A point is under a
    roof in its square, or in the rim of a square next to it where that
    cell's texel is opaque (`tiles`: the tiles atlas alpha; without it the
    rims count as clear)."""

    def __init__(self, height, cells, tiles=None):
        self.height, self.cells, self.tiles = height, cells, tiles

    def near(self, lo_u, hi_u, lo_v, hi_v):
        return any((col, row) in self.cells for col in _roof_cells(lo_u, hi_u) for row in _roof_cells(lo_v, hi_v))

    def covers(self, u, v):
        if (math.floor(u), math.floor(v)) in self.cells:
            return True
        if self.tiles is None:
            return False
        for col in _roof_cells(u, u):
            for row in _roof_cells(v, v):
                cell = self.cells.get((col, row))
                if cell is None:
                    continue
                x = min(max(int(cell[0] + ROOF_MARGIN + (u - col) * ROOF_CELL), cell[0]), cell[0] + ROOF_CELL + 2 * ROOF_MARGIN - 1)
                y = min(max(int(cell[1] + ROOF_MARGIN + (v - row) * ROOF_CELL), cell[1]), cell[1] + ROOF_CELL + 2 * ROOF_MARGIN - 1)
                if 0 <= y < len(self.tiles) and 0 <= x < len(self.tiles[y]) and self.tiles[y][x] >= 128:
                    return True
        return False


def _roof_grid(scene, tiles=None):
    """The scene's _Roofs, or None without roofs. tiles: the tiles atlas alpha
    (rows of 0..255), for the rims."""
    import base64
    projection = scene.get("proj") or {}
    data = scene.get("roof")
    if not isinstance(data, str) or "roofPx" not in projection:
        return None
    try:
        indices = struct.unpack("<10000H", base64.b64decode(data, validate=True))
    except (ValueError, struct.error):
        return None
    atlas = (scene.get("tiles") or {}).get("cells") or []
    cells = {(index % 100, index // 100): tuple(atlas[cell - 1]) if cell <= len(atlas) else (0, 0)
             for index, cell in enumerate(indices) if cell}
    if not cells:
        return None
    height = _finite(projection["roofPx"], "roof height") / _finite(projection["hpx"], "scene projection hpx")
    return _Roofs(height, cells, tiles if atlas else None)


def _roof_cells(lo, hi):
    """The columns (or rows) whose roof square, rim included, reaches into lo..hi."""
    return range(math.floor(lo - ROOF_RIM), math.floor(hi + ROOF_RIM) + 1)


def _over_roof(triangle, roofs):
    """Might a part of this triangle (three vertices of FLOATS) stand over a roof?"""
    if max(vertex[1] for vertex in triangle) <= roofs.height + ROOF_SLACK:
        return False
    us, vs = [vertex[0] + GL_MAP_OFFSET for vertex in triangle], [vertex[2] + GL_MAP_OFFSET for vertex in triangle]
    return roofs.near(min(us), max(us), min(vs), max(vs))


def _under_roofs(triangle, roofs):
    """The polygons (lists of vertices) of a triangle that stay for the free
    camera: all of it but what stands under a roof (_Roofs.covers) above its
    height. Every vertex value is affine on the triangle, so cuts interpolate
    it. The pieces are cut at the squares' and the rims' edges, and in a rim
    where its texels turn from clear to opaque along the piece."""
    height = roofs.height
    below = _clip(triangle, lambda p: height + ROOF_SLACK - p[1])
    above = [_clip(triangle, lambda p: p[1] - height - ROOF_SLACK)]
    above = [polygon for polygon in above if len(polygon) >= 3]
    extent = lambda polygon, axis: (min(p[axis] for p in polygon) + GL_MAP_OFFSET, max(p[axis] for p in polygon) + GL_MAP_OFFSET)
    centre = lambda polygon, axis: sum(p[axis] for p in polygon) / len(polygon) + GL_MAP_OFFSET

    def cut(polygons, axis, k):
        # only a polygon on both sides: one lying in the plane would come out twice
        return [part for polygon in polygons
                for part in (_split([polygon], lambda p: p[axis] + GL_MAP_OFFSET - k)
                             if extent(polygon, axis)[0] < k < extent(polygon, axis)[1] else [polygon])]

    for axis in (0, 2):
        cuts = sorted({k + edge for polygon in above for k in _roof_cells(*extent(polygon, axis))
                       for edge in (-ROOF_RIM, 0.0, ROOF_RIM)})
        for k in cuts:
            above = cut(above, axis, k)
    if roofs.tiles is not None:
        for axis, other in ((0, 2), (2, 0)):
            parts = []
            for polygon in above:
                lo, hi = extent(polygon, axis)
                at = centre(polygon, other)
                cover = lambda s: roofs.covers(s, at) if axis == 0 else roofs.covers(at, s)
                count = max(1, math.ceil((hi - lo) * ROOF_CELL))  # a sample a texel
                steps = [lo + (hi - lo) * (i + 0.5) / count for i in range(count)]
                pieces = [polygon]
                for a, b in zip(steps, steps[1:]):
                    if cover(a) != cover(b):
                        pieces = cut(pieces, axis, (a + b) / 2)
                parts.extend(pieces)
            above = parts
    kept = [below] if len(below) >= 3 else []
    kept.extend(polygon for polygon in above if not roofs.covers(centre(polygon, 0), centre(polygon, 2)))
    return kept


def _cut_by_roofs(batches, roofs, game=None, fly=None):
    """Leave out of the free camera's faces in `batches` what stands over a
    roof (_under_roofs). With `game`, `batches` are both cameras': the game's
    camera then keeps a cut face whole in `game`, the free camera's part goes
    to `fly`."""
    size = 3 * FLOATS
    for page in list(batches):
        values, keep = batches[page], []
        cut = keep if game is None else fly.setdefault(page, [])
        for start in range(0, len(values), size):
            face = values[start:start + size]
            triangle = [tuple(face[i * FLOATS:(i + 1) * FLOATS]) for i in range(3)]
            if not _over_roof(triangle, roofs):
                keep.extend(face)
                continue
            if game is not None:
                game.setdefault(page, []).extend(face)
            for polygon in _under_roofs(triangle, roofs):
                for index in range(1, len(polygon) - 1):
                    cut.extend(n for vertex in (polygon[0], polygon[index], polygon[index + 1]) for n in vertex)
        batches[page] = keep
    for target in (batches, fly or {}):
        for page in [page for page, values in target.items() if not values]:
            del target[page]


def _moved_strip(trim, moved):
    """Is this donor trim an end strip at a wall end that a corner moved?"""
    ua, va, ub, vb = trim[1:5]
    for slab, end in moved:
        lo, hi = slab.band()
        s, t = ((ua, ub), (va, vb)) if slab.axis == 0 else ((va, vb), (ua, ub))
        if abs(s[0] - end) < 0.06 and abs(s[1] - end) < 0.06 and lo - 0.06 <= min(t) and max(t) <= hi + 0.06:
            return True
    return False


def encode_wall_sidecar(scene, skip_props=frozenset(), skip_rests=frozenset(), map_objects=None, atlas=None, tiles=None):
    """Build the donor game view's wall triangles for one scene: what the game's
    camera sees of walls (fronts and tops) and the sheets that have no
    thickness. Backs and slab ends (wallRest, trims) are first-person only in
    the donor viewer, and in the game's view they would stick out at corners:
    they go into FLY_ONLY batches, which only the fly camera draws. (Cut-open
    upper parts are left out.) map_objects: the elevation's objects in map file
    order (the rows' serial numbers); the faces of each object name it in the
    table. atlas: the packed RGBA pages (alpha), so a corner's core takes an
    opaque texel. tiles: the tiles atlas alpha, for the roofs' rims."""
    if scene.get("format") != 6 or not one_map_elevation(scene):
        raise ValueError("wall sidecar expects one map elevation in scene format 6")
    pages_data = scene.get("pages")
    sprites = scene.get("sprites")
    # A map elevation may have no walls at all (sprites empty): the sidecar
    # then holds no batches.
    if not isinstance(pages_data, list) or not pages_data or not isinstance(sprites, list):
        raise ValueError("scene sprite page metadata is missing")
    projection = scene.get("proj")
    if not isinstance(projection, dict):
        raise ValueError("scene projection metadata is missing")
    hpx = _finite(projection.get("hpx"), "scene projection hpx")
    squ, sqv = projection.get("squ"), projection.get("sqv")
    if hpx <= 0 or not isinstance(squ, list) or len(squ) != 2 or not isinstance(sqv, list) or len(sqv) != 2:
        raise ValueError("invalid scene projection vectors")
    squ = [_finite(value, "scene projection vector") for value in squ]
    sqv = [_finite(value, "scene projection vector") for value in sqv]
    ray = [sqv[0], -squ[0], (sqv[0] * squ[1] - squ[0] * sqv[1]) / hpx]
    ray_length = math.sqrt(sum(component * component for component in ray))
    if ray_length == 0:
        raise ValueError("invalid zero-length scene camera ray")
    toward_camera = [component / ray_length for component in ray]
    pages = []
    for index, page in enumerate(pages_data):
        size = page.get("size") if isinstance(page, dict) else None
        if not isinstance(size, list) or len(size) != 2:
            raise ValueError(f"invalid sprite page {index} dimensions")
        width, height = (_int(value, "sprite page dimension") for value in size)
        if width <= 0 or height <= 0 or width > 65535 or height > 65535:
            raise ValueError(f"sprite page {index} dimensions are outside sidecar limits")
        pages.append((width, height))

    batches = _Batches()
    fly = _Batches()
    game = _Batches()
    table = {}

    def owner(serial):
        """The table row of map object `serial` (-1 without the map's objects)."""
        if map_objects is None or isinstance(serial, bool) or not isinstance(serial, int)                 or not 0 <= serial < len(map_objects):
            return -1.0
        obj = map_objects[serial]
        return float(table.setdefault((int(obj.tile), int(obj.fid)), len(table)))

    walls = scene.get("walls")
    if not isinstance(walls, list):
        raise ValueError("scene.walls is missing")
    proj = Projection(projection)
    for index, row in enumerate(walls):
        _record(row, 12, f"wall {index}")
        for value in row[1:5]:
            _finite(value, f"wall {index} position")
        _finite(row[10], f"wall {index} thickness")
    continuations = _continuations(walls)
    hidden_walls = set(scene.get("hiddenWalls") or [])
    px = (abs(squ[0]), abs(sqv[0]))  # picture columns per unit of wall along u, along v
    corners = _corner_fits(walls, hidden_walls, toward_camera,
                           lambda index: _int(sprites[_int(walls[index][0], "wall sprite")][3], "wall sprite width"), px)
    for index, row in enumerate(walls):
        batches.object = game.object = fly.object = owner(row[8])
        if row[10] > 0 and index in corners.keep:
            # A corner sets this wall's end: the game's camera keeps the row
            # whole, the free camera draws what stays.
            _wall_front(game, proj, sprites, pages, row, toward_camera, continuations[index])
            _wall_front(fly, proj, sprites, pages, row, toward_camera, continuations[index], corners.keep[index],
                        corners.top_keep[index], None if index in hidden_walls else fly, atlas)
        elif row[10] > 0:
            _wall_front(batches, proj, sprites, pages, row, toward_camera, continuations[index],
                        foot=None if index in hidden_walls else fly, atlas=atlas)
        else:
            _record(row, 7, f"thin wall {index}")
            sprite = _int(row[0], "thin wall sprite index")
            if not 0 <= sprite < len(sprites):
                raise ValueError(f"thin wall {index} sprite index is outside scene.sprites")
            _upright(batches, sprites, pages, sprite, *row[1:7], *sprites[sprite][1:5])

    # Backs of the solid walls (not those a solid or a form stands for in first
    # person) and the strips that close their ends and bridge where they meet.
    for index, row in enumerate(walls):
        if row[10] > 0 and index not in hidden_walls:
            fly.object = owner(row[8])
            _wall_back(fly, sprites, pages, row, hpx, corners.back_keep.get(index, (0.0, 1.0)), atlas)
    decor = _decor_pieces(atlas, sprites, hpx, walls, corners)
    # Both fronts of a corner with decor run on as they stand up to it.
    decor_ends = {key for index in decor for slab, out, twin, twin_out, _ in [corners.pieces[index]]
                  for key in ((id(slab), out), (id(twin), twin_out))}
    for slab, pool, s0, s1, back in corners.fills:
        fly.object = owner(walls[pool[0]][8])
        _corner_fill(fly, proj, sprites, pages, hpx, px, walls, pool, slab, s0, s1, back, atlas,
                     (id(slab), 1 if s1 > s0 else -1) in decor_ends)
    for index in decor:
        slab, out, twin, twin_out, _ = corners.pieces[index]
        fly.object = owner(walls[index][8])
        _corner_decor(fly, proj, sprites, pages, toward_camera, walls[index], slab, out, twin, twin_out)
    for slab, out, at, pool in corners.ends:
        fly.object = owner(walls[pool[0]][8])
        _corner_end(fly, proj, sprites, pages, hpx, px, toward_camera, walls, pool, slab, out, at, atlas)
    for slab, lender, s0, s1 in corners.caps:
        fly.object = owner(walls[lender][8])
        _corner_cap(fly, proj, sprites, pages, hpx, walls[lender], slab, s0, s1)
    solidity = {}
    solid = lambda slab: solidity.setdefault(id(slab), _solidity(walls, sprites, hpx, atlas, slab))
    for slab, other, lender in corners.cores:
        fly.object = owner(walls[lender][8])
        _corner_core(fly, proj, sprites, pages, hpx, toward_camera, walls[lender], slab, other, corners.extent,
                     solid, lambda wall: min(walls[index][9] for index in wall.rows), atlas)
    batches.object = game.object = fly.object = -1.0
    for index, row in enumerate(scene.get("trims") or []):
        _record(row, 11, f"trim {index}")
        if _moved_strip(row, corners.moved):
            continue
        sprite = _int(row[0], "trim sprite index")
        if not 0 <= sprite < len(sprites):
            raise ValueError(f"trim {index} sprite index is outside scene.sprites")
        _upright(fly, sprites, pages, sprite, *row[1:7], sprites[sprite][1] + row[7], sprites[sprite][2] + row[8],
                 row[9], row[10])

    roofs = _roof_grid(scene, tiles)
    if roofs is not None:
        _cut_by_roofs(fly, roofs)
        _cut_by_roofs(batches, roofs, game, fly)

    # Doors are not baked: they open and close, so the engine draws them live
    # as sprite cards with their current frame.

    # Furniture as solids (solids.py), and what of a drum or stem stays a card.
    # skip_*: rows whose object the engine keeps drawing live (it animates).
    boards = scene.get("boards") or []
    board_owner = lambda board: owner(boards[board][5]) if 0 <= board < len(boards) else -1.0
    # A solid's rest cards stand on its axis: they belong to its object.
    axis_owner = {}
    forced_axes = {(row[5], row[6]) for row in scene.get("props") or [] if len(row) > 14 and row[14]}
    for index, row in enumerate(scene.get("props") or []):
        axis_owner.setdefault((row[5], row[6]), board_owner(row[3]))
        if index in skip_props:
            continue
        batches.object = fly.object = game.object = board_owner(row[3])
        by_camera = {"both": batches, "fly": fly, "game": game}
        for page, corners, away, camera in solid_faces(proj, sprites, scene.get("boards") or [], row):
            _polygon(by_camera[camera], proj, page, pages[page], corners, away)
    fly.object = game.object = -1.0
    for index, row in enumerate(scene.get("rests") or []):
        if index in skip_rests:
            continue
        # The game's camera sees the card; the fly camera two crossed sheets,
        # each from both sides.
        game.object = fly.object = axis_owner.get((row[1], row[2]), -1.0)
        page, corners = rest_card(proj, sprites, row)
        seen = row[5] if len(row) > 5 else 0     # 1: of a solid the fly camera alone sees, 2: the game's
        if (row[1], row[2]) not in forced_axes and seen != 1:  # the game's camera sees that solid's card whole
            _polygon(game, proj, page, pages[page], corners)
        for page, corners in rest_cross(proj, sprites, row) if seen != 2 else ():
            # The sheet across the screen is a line to the game's camera, so
            # it is wound both ways without asking that camera.
            for wound in (corners, corners[::-1]):
                for index in range(1, len(wound) - 1):
                    _quad_triangle(fly, page, pages[page], (wound[0], wound[index], wound[index + 1]))
    fly.object = game.object = -1.0

    groups = [(0, page, batches[page]) for page in sorted(batches)] + [(FLY_ONLY, page, fly[page]) for page in sorted(fly)] \
        + [(GAME_ONLY, page, game[page]) for page in sorted(game)]
    total_triangles = sum(len(values) // (3 * FLOATS) for _, _, values in groups)
    if total_triangles > 0xFFFFFFFF or len(groups) > 0xFFFFFFFF:
        raise ValueError("wall sidecar exceeds binary format limits")
    if len(pages) > 0xFFFFFFFF:
        raise ValueError("scene has too many sprite atlas pages")
    payload = bytearray(HEADER.pack(MAGIC, VERSION, HEADER.size, len(pages), len(groups), total_triangles, len(table)))
    for flags, page, values in groups:
        if len(values) % (3 * FLOATS):
            raise AssertionError("internal wall sidecar vertex alignment error")
        width, height = pages[page]
        triangle_count = len(values) // (3 * FLOATS)
        payload.extend(BATCH.pack(page, width, height, triangle_count, flags))
        payload.extend(struct.pack(f"<{len(values)}f", *values))
    for (tile, fid), _ in sorted(table.items(), key=lambda item: item[1]):
        payload.extend(OBJECT.pack(tile, fid))
    return bytes(payload)


def decode_wall_sidecar(data):
    """Validate and decode the binary contract; shared by tests and tooling."""
    if len(data) < HEADER.size:
        raise ValueError("truncated wall sidecar header")
    magic, version, header_size, page_count, batch_count, triangle_count, object_count = HEADER.unpack_from(data)
    if magic != MAGIC or version != VERSION or header_size != HEADER.size:
        raise ValueError("unsupported wall sidecar magic/version/header")
    if page_count == 0 or batch_count > 3 * page_count:
        raise ValueError("invalid wall sidecar atlas/batch count")
    offset = HEADER.size
    batches = []
    seen_pages = set()
    observed_triangles = 0
    for _ in range(batch_count):
        if offset + BATCH.size > len(data):
            raise ValueError("truncated wall sidecar batch")
        page, width, height, count, flags = BATCH.unpack_from(data, offset)
        offset += BATCH.size
        if page >= page_count or (page, flags) in seen_pages or flags not in (0, FLY_ONLY, GAME_ONLY) \
                or width == 0 or height == 0 or count == 0:
            raise ValueError("invalid wall sidecar page batch")
        seen_pages.add((page, flags))
        vertex_count = count * 3
        byte_count = vertex_count * VERTEX.size
        if offset + byte_count > len(data):
            raise ValueError("truncated wall sidecar vertices")
        vertices = [VERTEX.unpack_from(data, offset + i * VERTEX.size) for i in range(vertex_count)]
        offset += byte_count
        for x, y, z, u, v, dx, dy, dz, owner in vertices:
            if not all(math.isfinite(value) for value in (x, y, z, u, v, dx, dy, dz)):
                raise ValueError("wall sidecar has non-finite vertex data")
            if owner != -1.0 and not (owner == int(owner) and 0 <= owner < object_count):
                raise ValueError("wall sidecar vertex names an object outside its table")
            if not (0 <= u <= 1 and 0 <= v <= 1):
                raise ValueError("wall sidecar UV is outside normalized atlas bounds")
        batches.append({"page": page, "size": (width, height), "vertices": vertices, "triangles": count,
                        "fly_only": flags == FLY_ONLY, "game_only": flags == GAME_ONLY})
        observed_triangles += count
    if offset + object_count * OBJECT.size > len(data):
        raise ValueError("truncated wall sidecar object table")
    objects = [OBJECT.unpack_from(data, offset + i * OBJECT.size) for i in range(object_count)]
    offset += object_count * OBJECT.size
    if offset != len(data) or observed_triangles != triangle_count:
        raise ValueError("wall sidecar trailing bytes or triangle count mismatch")
    return {"version": version, "triangles": triangle_count, "batches": batches, "objects": objects}
