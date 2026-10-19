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


def _wall_front(batches, sprites, pages, row, hpx, toward_camera):
    sprite, ua, va, ub, vb, z0, z1, _, _, zc = row[:10]
    sprite = _int(sprite, "wall sprite")
    if not 0 <= sprite < len(sprites):
        raise ValueError(f"wall sprite index {sprite} is outside scene.sprites")
    page, x, y, w, h, *_ = sprites[sprite]
    page = _int(page, "wall sprite page")
    if not 0 <= page < len(pages):
        raise ValueError(f"wall sprite page {page} is outside scene.pages")
    x, y, w, h = (_int(value, "wall sprite rectangle") for value in (x, y, w, h))
    z0, z1, zc = (_finite(value, "wall height") for value in (z0, z1, zc))
    top = (z1 - zc) * hpx
    _upright(batches, sprites, pages, sprite, ua, va, ub, vb, z0, zc,
             x, y + top, w, h - top)
    if top <= 0:
        return
    k = toward_camera
    if k[2] == 0:
        raise ValueError("donor projection has a horizontal camera ray")
    far = (z1 - zc) / k[2]
    du, dv = -k[0] * far, -k[1] * far
    # The top keeps the depth of the front plane it was painted on: what the
    # game drew the wall over (a lamp post right behind it) stays hidden.
    home = [-du, -dv, z1 - zc]
    _quad(batches, page, pages[page], [
        [ua, va, zc, x, y + top], [ub, vb, zc, x + w, y + top],
        [ub + du, vb + dv, zc, x + w, y, *home], [ua + du, va + dv, zc, x, y, *home],
    ])


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
    for index, row in enumerate(walls):
        _record(row, 12, f"wall {index}")
        thickness = _finite(row[10], f"wall {index} thickness")
        if thickness > 0:
            _wall_front(batches, sprites, pages, row, hpx, toward_camera)
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
    proj = Projection(projection)
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
