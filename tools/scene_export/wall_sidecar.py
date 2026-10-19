"""Versioned donor-surface triangles for the CE static wall preview."""

from __future__ import annotations

import math
import struct

from scene_check import one_map_elevation
from solids import Projection, rest_card, screen_area, solid_faces


MAGIC = b"F2WL"
VERSION = 2
HEADER = struct.Struct("<4sHHIII")  # magic, version, header bytes, atlas pages, batches, triangles
BATCH = struct.Struct("<IIII")  # atlas page, width, height, triangles
# GL position xyz, top-down normalized atlas uv, GL point whose depth the vertex
# takes (itself, except on wall tops: the front-plane point its texel was painted on)
VERTEX = struct.Struct("<8f")
FLOATS = 8
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
                       u + hu - GL_MAP_OFFSET, z + hz, v + hv - GL_MAP_OFFSET))
    target = batches.setdefault(page, [])
    for index in (0, 1, 2, 0, 2, 3):
        target.extend(points[index])


def _upright(batches, sprites, pages, sprite_id, ua, va, ub, vb, z0, z1, tx, ty, tw, th):
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
    _quad(batches, page, (image_w, image_h), [
        [ua, va, z0, tx, ty + th], [ub, vb, z0, tx + tw, ty + th],
        [ub, vb, z1, tx + tw, ty], [ua, va, z1, tx, ty],
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


def _wall_front(batches, proj, sprites, pages, row, toward_camera, continues):
    """The front up to the cap line, then the slab's top as a real horizontal
    face as deep as the wall is thick, and what of the picture is higher than
    that top as an upright face on the slab's far side. Each point samples the
    texel the game drew it on, so without perspective the picture is the
    game's. Where the wall goes on in the same line (continues: a end, b end)
    the piece ends where its picture ends; a free end is cut square, as the
    slab is."""
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
             x, y + top, w, h - top)
    if top <= 0:
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
    if rise > 1e-6:
        faces.append(ends([at(-reach, thick, zc), at(reach, thick, zc),
                           at(reach, thick, zc + rise), at(-reach, thick, zc + rise)], (False, False)))
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


def _polygon(batches, proj, page, page_size, corners):
    """A convex polygon as a triangle fan, wound counter-clockwise on the
    screen like the wall fronts (they are drawn with back faces culled)."""
    area = screen_area(proj, corners)
    if abs(area) < 1e-6:
        return
    if area < 0:
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
                       u + hu - GL_MAP_OFFSET, z + hz, v + hv - GL_MAP_OFFSET))


def encode_wall_sidecar(scene, skip_props=frozenset(), skip_rests=frozenset()):
    """Build the donor game view's wall triangles for one scene: what the game's
    camera sees of walls (fronts and tops) and the sheets that have no
    thickness. Backs, cut-open upper parts and slab ends (wallRest, trims) are
    first-person only in the donor viewer; in the game's view they stick out
    at corners."""
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

    batches = {}
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
    for index, row in enumerate(walls):
        if row[10] > 0:
            _wall_front(batches, proj, sprites, pages, row, toward_camera, continuations[index])
        else:
            _record(row, 7, f"thin wall {index}")
            sprite = _int(row[0], "thin wall sprite index")
            if not 0 <= sprite < len(sprites):
                raise ValueError(f"thin wall {index} sprite index is outside scene.sprites")
            _upright(batches, sprites, pages, sprite, *row[1:7], *sprites[sprite][1:5])

    # Doors are not baked: they open and close, so the engine draws them live
    # as sprite cards with their current frame.

    # Furniture as solids (solids.py), and what of a drum or stem stays a card.
    # skip_*: rows whose object the engine keeps drawing live (it animates).
    for index, row in enumerate(scene.get("props") or []):
        if index in skip_props:
            continue
        for page, corners in solid_faces(proj, sprites, scene.get("boards") or [], row):
            _polygon(batches, proj, page, pages[page], corners)
    for index, row in enumerate(scene.get("rests") or []):
        if index in skip_rests:
            continue
        page, corners = rest_card(proj, sprites, row)
        _polygon(batches, proj, page, pages[page], corners)

    total_triangles = sum(len(values) // (3 * FLOATS) for values in batches.values())
    if total_triangles > 0xFFFFFFFF or len(batches) > 0xFFFFFFFF:
        raise ValueError("wall sidecar exceeds binary format limits")
    if len(pages) > 0xFFFFFFFF:
        raise ValueError("scene has too many sprite atlas pages")
    payload = bytearray(HEADER.pack(MAGIC, VERSION, HEADER.size, len(pages), len(batches), total_triangles))
    for page in sorted(batches):
        values = batches[page]
        if len(values) % (3 * FLOATS):
            raise AssertionError("internal wall sidecar vertex alignment error")
        width, height = pages[page]
        triangle_count = len(values) // (3 * FLOATS)
        payload.extend(BATCH.pack(page, width, height, triangle_count))
        payload.extend(struct.pack(f"<{len(values)}f", *values))
    return bytes(payload)


def decode_wall_sidecar(data):
    """Validate and decode the binary contract; shared by tests and tooling."""
    if len(data) < HEADER.size:
        raise ValueError("truncated wall sidecar header")
    magic, version, header_size, page_count, batch_count, triangle_count = HEADER.unpack_from(data)
    if magic != MAGIC or version != VERSION or header_size != HEADER.size:
        raise ValueError("unsupported wall sidecar magic/version/header")
    if page_count == 0 or batch_count > page_count:
        raise ValueError("invalid wall sidecar atlas/batch count")
    offset = HEADER.size
    batches = []
    seen_pages = set()
    observed_triangles = 0
    for _ in range(batch_count):
        if offset + BATCH.size > len(data):
            raise ValueError("truncated wall sidecar batch")
        page, width, height, count = BATCH.unpack_from(data, offset)
        offset += BATCH.size
        if page >= page_count or page in seen_pages or width == 0 or height == 0 or count == 0:
            raise ValueError("invalid wall sidecar page batch")
        seen_pages.add(page)
        vertex_count = count * 3
        byte_count = vertex_count * VERTEX.size
        if offset + byte_count > len(data):
            raise ValueError("truncated wall sidecar vertices")
        vertices = [VERTEX.unpack_from(data, offset + i * VERTEX.size) for i in range(vertex_count)]
        offset += byte_count
        for x, y, z, u, v, *depth_at in vertices:
            if not all(math.isfinite(value) for value in (x, y, z, u, v, *depth_at)):
                raise ValueError("wall sidecar has non-finite vertex data")
            if not (0 <= u <= 1 and 0 <= v <= 1):
                raise ValueError("wall sidecar UV is outside normalized atlas bounds")
        batches.append({"page": page, "size": (width, height), "vertices": vertices, "triangles": count})
        observed_triangles += count
    if offset != len(data) or observed_triangles != triangle_count:
        raise ValueError("wall sidecar trailing bytes or triangle count mismatch")
    return {"version": version, "triangles": triangle_count, "batches": batches}
