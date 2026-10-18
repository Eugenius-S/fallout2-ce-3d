"""Versioned list of map objects that walls.bin already draws.

Walls and the scenery panels that stand in a wall plane are baked into
walls.bin. The engine draws every other live object as a sprite card and
skips the objects named here, so nothing is drawn twice. Doors are not baked:
they open, so the engine draws them live.
"""

from __future__ import annotations

import struct


MAGIC = b"F2BK"
VERSION = 1
HEADER = struct.Struct("<4sHHI")  # magic, version, header bytes, entries
ENTRY = struct.Struct("<iI")  # object tile, object fid
MAX_ENTRIES = 1000000


def baked_objects(scene, map_objects):
    """(tile, fid) of every object behind a walls.bin row; map_objects is the
    elevation's object list in map file order (the rows' serial numbers)."""
    walls = scene.get("walls")
    if not isinstance(walls, list):
        raise ValueError("scene.walls is missing")
    entries = set()
    for index, row in enumerate(walls):
        if not isinstance(row, list) or len(row) < 9:
            raise ValueError(f"invalid wall {index} record")
        serial = row[8]
        if isinstance(serial, bool) or not isinstance(serial, int) or not 0 <= serial < len(map_objects):
            raise ValueError(f"wall {index} names an object outside the map")
        obj = map_objects[serial]
        entries.add((obj.tile, obj.fid))
    return sorted(entries)


def encode_baked_sidecar(entries):
    entries = sorted(set(entries))
    if len(entries) > MAX_ENTRIES:
        raise ValueError("baked object list exceeds format limits")
    payload = bytearray(HEADER.pack(MAGIC, VERSION, HEADER.size, len(entries)))
    for tile, fid in entries:
        if not 0 <= tile < 200 * 200:
            raise ValueError(f"baked object tile {tile} is outside the hex grid")
        if not 0 <= fid <= 0xFFFFFFFF:
            raise ValueError(f"baked object fid {fid} is out of range")
        payload.extend(ENTRY.pack(tile, fid))
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
    if any(not 0 <= tile < 200 * 200 for tile, _ in entries):
        raise ValueError("baked sidecar names a tile outside the hex grid")
    return entries
