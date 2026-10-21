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


def _wall_back(batches, sprites, pages, row, hpx):
    """What the game's camera does not see of a solid wall (donor wallRest): its
    far side, `thick` behind the front, the same outline seen from behind up to
    the cap line, in the picture `back` (-1: the wall's own below its top band),
    mirrored as seen from that side."""
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
    _upright(batches, sprites, pages, picture, ub - nu, vb - nv, ua - nu, va - nv, z0, zc, x + w, y, -w, h)


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
    for index, row in enumerate(walls):
        batches.object = owner(row[8])
        if row[10] > 0:
            _wall_front(batches, proj, sprites, pages, row, toward_camera, continuations[index])
        else:
            _record(row, 7, f"thin wall {index}")
            sprite = _int(row[0], "thin wall sprite index")
            if not 0 <= sprite < len(sprites):
                raise ValueError(f"thin wall {index} sprite index is outside scene.sprites")
            _upright(batches, sprites, pages, sprite, *row[1:7], *sprites[sprite][1:5])

    # Backs of the solid walls (not those a solid or a form stands for in first
    # person) and the strips that close their ends and bridge where they meet.
    hidden_walls = set(scene.get("hiddenWalls") or [])
    for index, row in enumerate(walls):
        if row[10] > 0 and index not in hidden_walls:
            fly.object = owner(row[8])
            _wall_back(fly, sprites, pages, row, hpx)
    fly.object = -1.0
    for index, row in enumerate(scene.get("trims") or []):
        _record(row, 11, f"trim {index}")
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
