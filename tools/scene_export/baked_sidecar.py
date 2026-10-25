"""Versioned list of map objects the engine's live sprite cards treat apart.

Walls, the scenery panels in wall planes and furniture solids are baked into
walls.bin; the engine skips those objects (SKIP). Things standing on a solid
(an item on a table) are lifted onto it (LIFT): moved by (du, dv) on the floor
and dz up, along the game's view ray, so the game's camera still sees them on
the same pixels. Items and scenery the donor lays on the floor (low piles of
junk, hay, goo: things.py `stand` -> "flat", scene.decals) are drawn flat
(FLAT), as the game draws its FLAT objects. The cards no more than LOW_HEIGHT
pixels tall (a cabbage, a tin) lie flat only when the camera flies (FLY_FLAT):
the game's camera sees a flat card on other pixels. A card that a solid made
by name stands for (overrides.json) is skipped only in flight (FLY_SKIP). Doors and gates stay live (they
open), but on their wall's plane (PLANE: GL x, z of a point of it and dz
the plane's direction, radians from +u toward +v) when the camera flies.
Critters are not here: they live and die.
"""

from __future__ import annotations

import math
import struct


MAGIC = b"F2BK"
VERSION = 6
HEADER = struct.Struct("<4sHHI")  # magic, version, header bytes, entries
ENTRY = struct.Struct("<iIIfff")  # object tile, object fid, mode, du, dv, dz
SKIP = 0
LIFT = 1
FLAT = 2
PLANE = 3
FLY_FLAT = 4
FLY_SKIP = 5  # a card a solid made by name stands for, but only in flight
NOCAST = 0x80000000  # mode's high bit: the object's cards throw no shadow (a plant's crown)
MODE_MASK = 0x7FFFFFFF
GL_MAP_OFFSET = 50.0  # GL x, z = u, v less this (wall_sidecar.py)
LOW_HEIGHT = 20  # px: a card this low (and wider than tall) lies flat
FLAT_TYPES = (0, 2)  # items, scenery (fid >> 24)
MAX_ENTRIES = 1000000


def _object(map_objects, serial, label):
    if isinstance(serial, bool) or not isinstance(serial, int) or not 0 <= serial < len(map_objects):
        raise ValueError(f"{label} names an object outside the map")
    return map_objects[serial]


def baked_objects(scene, map_objects, live_boards=frozenset()):
    """Entries (tile, fid, mode, du, dv, dz). map_objects is the elevation's
    object list in map file order (the rows' serial numbers); live_boards are
    scene.boards rows the engine keeps drawing as cards (they animate)."""
    walls = scene.get("walls")
    if not isinstance(walls, list):
        raise ValueError("scene.walls is missing")
    entries = {}
    for index, row in enumerate(walls):
        if not isinstance(row, list) or len(row) < 9:
            raise ValueError(f"invalid wall {index} record")
        obj = _object(map_objects, row[8], f"wall {index}")
        entries[(obj.tile, obj.fid)] = (SKIP, 0.0, 0.0, 0.0)

    boards = scene.get("boards") or []
    # Cards a solid stands for. Forms (tents, set pieces) are not drawn, so
    # with any of them only the boards a solid names are left out.
    replaced = set(scene.get("hidden") or []) if not scene.get("forms") else {
        row[3] for row in scene.get("props") or [] if row[3] >= 0}
    forced = {row[3] for row in scene.get("props") or [] if len(row) > 14 and row[14] and row[3] >= 0}
    for board in sorted(replaced - set(live_boards)):
        if not 0 <= board < len(boards):
            raise ValueError(f"hidden board {board} is outside scene.boards")
        obj = _object(map_objects, boards[board][5], f"board {board}")
        entries[(obj.tile, obj.fid)] = (FLY_SKIP if board in forced else SKIP, 0.0, 0.0, 0.0)
    for row in scene.get("lifts") or []:
        board, du, dv, dz = row[:4]
        if board in replaced and board not in live_boards:
            continue
        obj = _object(map_objects, boards[board][5], f"lifted board {board}")
        entries.setdefault((obj.tile, obj.fid), (LIFT, float(du), float(dv), float(dz)))
    for index, row in enumerate(scene.get("decals") or []):
        if not isinstance(row, list) or len(row) < 5:
            raise ValueError(f"invalid decal {index} record")
        obj = _object(map_objects, row[4], f"decal {index}")
        if (int(obj.fid) >> 24) & 0xF in FLAT_TYPES:
            entries.setdefault((obj.tile, obj.fid), (FLAT, 0.0, 0.0, 0.0))
    sprites = scene.get("sprites") or []
    for index, row in enumerate(boards):
        if index in replaced or index in live_boards or not 0 <= row[0] < len(sprites):
            continue
        width, height = sprites[row[0]][3], sprites[row[0]][4]
        obj = _object(map_objects, row[5], f"board {index}")
        if height <= LOW_HEIGHT and height < width and (int(obj.fid) >> 24) & 0xF in FLAT_TYPES:
            entries.setdefault((obj.tile, obj.fid), (FLY_FLAT, 0.0, 0.0, 0.0))
    for index, row in enumerate(scene.get("doors") or []):
        if not isinstance(row, list) or len(row) < 9:
            raise ValueError(f"invalid door {index} record")
        obj = _object(map_objects, row[8], f"door {index}")
        ua, va, ub, vb = (float(value) for value in row[1:5])
        entries.setdefault((obj.tile, obj.fid), (PLANE, ua - GL_MAP_OFFSET, va - GL_MAP_OFFSET,
                                                 math.atan2(vb - va, ub - ua)))
    # A plant's crown throws no shadow (task 045's follow-up): the mark rides
    # the mode's high bit, on whatever the object's entry is.
    for board in scene.get("plants") or []:
        if not isinstance(board, int) or isinstance(board, bool) or not 0 <= board < len(boards):
            raise ValueError(f"plant board {board} is outside scene.boards")
        obj = _object(map_objects, boards[board][5], f"plant board {board}")
        key = (obj.tile, obj.fid)
        if key in entries:
            entries[key] = (entries[key][0] | NOCAST,) + entries[key][1:]
    return sorted((tile, fid, *rest) for (tile, fid), rest in entries.items())


def encode_baked_sidecar(entries):
    entries = sorted(set(entries))
    if len(entries) > MAX_ENTRIES:
        raise ValueError("baked object list exceeds format limits")
    payload = bytearray(HEADER.pack(MAGIC, VERSION, HEADER.size, len(entries)))
    for tile, fid, mode, du, dv, dz in entries:
        if not 0 <= tile < 200 * 200:
            raise ValueError(f"baked object tile {tile} is outside the hex grid")
        if not 0 <= fid <= 0xFFFFFFFF:
            raise ValueError(f"baked object fid {fid} is out of range")
        if (mode & MODE_MASK) not in (SKIP, LIFT, FLAT, PLANE, FLY_FLAT, FLY_SKIP):
            raise ValueError(f"baked object mode {mode} is unknown")
        payload.extend(ENTRY.pack(tile, fid, mode, du, dv, dz))
    return bytes(payload)


def decode_baked_sidecar(payload):
    if len(payload) < HEADER.size:
        raise ValueError("baked sidecar is truncated")
    magic, version, header_size, count = HEADER.unpack_from(payload)
    if magic != MAGIC or version != VERSION or header_size != HEADER.size or count > MAX_ENTRIES:
        raise ValueError("baked sidecar magic/version/header is unsupported")
    if len(payload) != HEADER.size + count * ENTRY.size:
        raise ValueError("baked sidecar has a bad length")
    entries = [ENTRY.unpack_from(payload, HEADER.size + index * ENTRY.size) for index in range(count)]
    if any(not 0 <= entry[0] < 200 * 200 for entry in entries):
        raise ValueError("baked sidecar names a tile outside the hex grid")
    if any((entry[2] & MODE_MASK) not in (SKIP, LIFT, FLAT, PLANE, FLY_FLAT, FLY_SKIP) for entry in entries):
        raise ValueError("baked sidecar has an unknown mode")
    return entries
