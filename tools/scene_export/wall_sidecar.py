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


def _wall_back(batches, sprites, pages, row, hpx, keep=(0.0, 1.0)):
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
    _upright(batches, sprites, pages, picture, ub - nu, vb - nv, ua - nu, va - nv, z0, zc, x + w, y, -w, h,
             (1.0 - min(keep[1], 1.0), 1.0 - max(keep[0], 0.0)))


def _wall_front(batches, proj, sprites, pages, row, toward_camera, continues, keep=(0.0, 1.0)):
    """The front up to the cap line, then the slab's top as a real horizontal
    face as deep as the wall is thick, and what of the picture is higher than
    that top as an upright face on the slab's far side. Each point samples the
    texel the game drew it on, so without perspective the picture is the
    game's. Where the wall goes on in the same line (continues: a end, b end)
    the piece ends where its picture ends; a free end is cut square, as the
    slab is. `keep`: the stretch of the wall that stays (fractions from a to
    b), where a corner moved its end (_corner_fits)."""
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
    if top <= 0 or keep[1] <= keep[0]:
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
    if rise > 1e-6 and keep == (0.0, 1.0):
        faces.append(ends([at(-reach, thick, zc), at(reach, thick, zc),
                           at(reach, thick, zc + rise), at(-reach, thick, zc + rise)], (False, False)))
    if keep != (0.0, 1.0):
        faces = [_clip(_clip(face, lambda p: along(p) - keep[0] * length), lambda p: keep[1] * length - along(p))
                 for face in faces if len(face) >= 3]
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


# Corners (task 071). The game draws a corner as one picture, and the donor
# lays each picture on its own wall's line: at a corner one wall runs on past
# the other by up to a hex and the other stops short of it, so the free camera
# sees a stub and a notch. Each end of a thick wall that meets another at a
# right angle is set where it belongs: at an L one wall reaches across the
# other's far face and the other stops at its near face; at a T the stem stops
# at the near face. An overrun is cut off for the free camera (the game's
# camera keeps the row whole), a shortfall is filled with a block of the
# wall's own material. The game's view stays as it was.
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
    """What the corner pass changes for the free camera. keep {wall row: (from,
    to)}: the stretch of the row it draws, as fractions from the row's a end
    (beyond 0 or 1: only the row's top is cut there); fills [(slab, lender row,
    s from, s to)]: blocks of the wall's own material; faces [(slab, outward,
    s, row)]: an end face in the picture of the wall whose plane it lies in;
    caps [(slab, row, s from, s to)]: the top at the corner, one flat piece;
    moved [(slab, old end)]: the ends whose donor end strips no longer stand at
    the wall's end."""

    def __init__(self):
        self.keep, self.fills, self.faces, self.caps, self.moved = {}, [], [], [], []


def _corner_fits(walls, hidden, toward_camera):
    slabs = _slabs(walls, hidden, toward_camera)
    gap = lambda x, lo, hi: max(lo - x, 0.0, x - hi)
    far = lambda slab, out: slab.band()[1] if out > 0 else slab.band()[0]
    near = lambda slab, out: slab.band()[0] if out > 0 else slab.band()[1]
    targets = {}
    for a in slabs:
        a0, a1 = a.band()
        for end, out in ((a.lo, -1), (a.hi, 1)):
            if (id(a), out) in targets:
                continue
            met = [(gap(end, *b.band()), index, b) for index, b in enumerate(slabs) if b.axis != a.axis
                   and gap(end, *b.band()) <= CORNER_REACH and b.hi >= a0 - CORNER_REACH and b.lo <= a1 + CORNER_REACH]
            if not met:
                continue
            b = min(met, key=lambda item: item[:2])[2]
            if b.lo < a0 - CORNER_REACH and b.hi > a1 + CORNER_REACH:
                targets[(id(a), out)] = (a, out, near(b, out), None)  # a T: the stem stops at b
                continue
            if b.hi <= a1 + CORNER_REACH and b.lo >= a0 - CORNER_REACH:
                continue  # all of b lies at a's end: no corner to read
            b_out = 1 if b.hi <= a1 + CORNER_REACH else -1
            b_end = b.hi if b_out > 0 else b.lo
            # The wall that runs on further past the other reaches across it;
            # its end face then lies in the other's plane.
            if out * (end - far(b, out)) >= b_out * (b_end - far(a, b_out)):
                targets[(id(a), out)] = (a, out, far(b, out), b)
                targets.setdefault((id(b), b_out), (b, b_out, near(a, b_out), None))
            else:
                targets[(id(a), out)] = (a, out, near(b, out), None)
                targets.setdefault((id(b), b_out), (b, b_out, far(a, b_out), a))
    corners = _Corners()
    corners.targets = list(targets.values())
    for slab, out, target, other in targets.values():
        end = slab.hi if out > 0 else slab.lo
        axis = slab.axis
        s_of = lambda row, at: row[1 + 2 * at + axis]  # the row's s at its a (0) or b (1) end
        span = lambda index: sorted((s_of(walls[index], 0), s_of(walls[index], 1)))
        lender = max(slab.rows, key=lambda index: span(index)[1] if out > 0 else -span(index)[0])
        # Every row near this end stops at the target, its top included: a
        # top runs on past its picture's edge and would stick out of the corner.
        for index in slab.rows:
            lo_s, hi_s = span(index)
            if out * ((hi_s if out > 0 else lo_s) - target) < -2 * CORNER_REACH:
                continue
            row = walls[index]
            sa, sb = s_of(row, 0), s_of(row, 1)
            if sb == sa:
                continue
            cut = (target - sa) / (sb - sa)
            lo, hi = corners.keep.get(index, (-math.inf, math.inf))
            if (sb - sa) * out > 0:
                hi = min(hi, cut)
            else:
                lo = max(lo, cut)
            corners.keep[index] = (lo, hi)
        overrun = out * (end - target)
        if overrun < -CORNER_SLACK:
            corners.fills.append((slab, lender, end, target))
        if abs(overrun) > CORNER_SLACK:
            corners.moved.append((slab, end))
        if other is not None:
            # The other wall's row nearest the corner lends the end face.
            middle = sum(slab.band()) / 2
            other_s = lambda index: (walls[index][1 + other.axis], walls[index][3 + other.axis])
            face_row = min(other.rows, key=lambda index: gap(middle, *sorted(other_s(index))))
            corners.faces.append((slab, out, target, face_row))
        # The cap stays within what is left of the slab: a short corner piece
        # may have both of its ends set.
        other_end = targets.get((id(slab), -out))
        stop = other_end[2] if other_end else (slab.lo if out > 0 else slab.hi)
        reach = target - out * max(2 * slab.thick, 0.35)
        corners.caps.append((slab, lender, max(reach, stop) if out > 0 else min(reach, stop), target))
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


def _corner_fill(batches, proj, sprites, pages, hpx, px, row, slab, s0, s1):
    """A block of the wall's own material between s0 and s1 along `slab`: its
    front and back, a strip of columns from the picture's edge at that end.
    px: picture columns per unit along u and along v."""
    page, x, y, w, h, top = _picture(sprites, row, hpx)
    z0, zc, thick = row[5], row[9], slab.thick
    c0, c1 = _strip(row, slab.axis, s1, px[slab.axis], w, x, abs(s1 - s0))
    lo, hi = sorted((s0, s1))
    lo, hi = lo - CORNER_OVERLAP, hi + CORNER_OVERLAP
    p = slab.point
    for depth, away in ((0.0, False), (thick, True)):
        corners = [[*p(lo, depth), z0, c0, y + h], [*p(hi, depth), z0, c1, y + h],
                   [*p(hi, depth), zc, c1, y + top], [*p(lo, depth), zc, c0, y + top]]
        _polygon(batches, proj, page, pages[page], corners, away)


def _corner_face(batches, proj, sprites, pages, hpx, px, toward_camera, row, slab, out, s):
    """The end face of `slab` at s, across its thickness, in the picture of the
    wall `row` whose plane it lies in, so the two read as one face."""
    page, x, y, w, h, top = _picture(sprites, row, hpx)
    axis = 1 - slab.axis  # the face runs along the other wall
    c0, c1 = _strip(row, axis, sum(slab.band()) / 2, px[axis], w, x, slab.thick)
    z0, zc = row[5], row[9]
    p = slab.point
    near, far = -CORNER_OVERLAP, slab.thick + CORNER_OVERLAP
    corners = [[*p(s, near), z0, c0, y + h], [*p(s, far), z0, c1, y + h],
               [*p(s, far), zc, c1, y + top], [*p(s, near), zc, c0, y + top]]
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


def _moved_strip(trim, moved):
    """Is this donor trim an end strip at a wall end that a corner moved?"""
    ua, va, ub, vb = trim[1:5]
    for slab, end in moved:
        lo, hi = slab.band()
        s, t = ((ua, ub), (va, vb)) if slab.axis == 0 else ((va, vb), (ua, ub))
        if abs(s[0] - end) < 0.06 and abs(s[1] - end) < 0.06 and lo - 0.06 <= min(t) and max(t) <= hi + 0.06:
            return True
    return False


def encode_wall_sidecar(scene, skip_props=frozenset(), skip_rests=frozenset(), map_objects=None):
    """Build the donor game view's wall triangles for one scene: what the game's
    camera sees of walls (fronts and tops) and the sheets that have no
    thickness. Backs and slab ends (wallRest, trims) are first-person only in
    the donor viewer, and in the game's view they would stick out at corners:
    they go into FLY_ONLY batches, which only the fly camera draws. (Cut-open
    upper parts are left out.) map_objects: the elevation's objects in map file
    order (the rows' serial numbers); the faces of each object name it in the
    table."""
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
    corners = _corner_fits(walls, hidden_walls, toward_camera)
    px = (abs(squ[0]), abs(sqv[0]))  # picture columns per unit of wall along u, along v
    for index, row in enumerate(walls):
        batches.object = game.object = fly.object = owner(row[8])
        if row[10] > 0 and index in corners.keep:
            # A corner sets this wall's end: the game's camera keeps the row
            # whole, the free camera draws what stays.
            _wall_front(game, proj, sprites, pages, row, toward_camera, continuations[index])
            _wall_front(fly, proj, sprites, pages, row, toward_camera, continuations[index], corners.keep[index])
        elif row[10] > 0:
            _wall_front(batches, proj, sprites, pages, row, toward_camera, continuations[index])
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
            _wall_back(fly, sprites, pages, row, hpx, corners.keep.get(index, (0.0, 1.0)))
    for slab, lender, s0, s1 in corners.fills:
        fly.object = owner(walls[lender][8])
        _corner_fill(fly, proj, sprites, pages, hpx, px, walls[lender], slab, s0, s1)
    for slab, out, at, lender in corners.faces:
        fly.object = owner(walls[lender][8])
        _corner_face(fly, proj, sprites, pages, hpx, px, toward_camera, walls[lender], slab, out, at)
    for slab, lender, s0, s1 in corners.caps:
        fly.object = owner(walls[lender][8])
        _corner_cap(fly, proj, sprites, pages, hpx, walls[lender], slab, s0, s1)
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
        if (row[1], row[2]) not in forced_axes:  # the game's camera sees that solid's card whole
            _polygon(game, proj, page, pages[page], corners)
        for page, corners in rest_cross(proj, sprites, row):
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
