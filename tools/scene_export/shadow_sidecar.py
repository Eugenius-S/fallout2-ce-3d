"""Versioned shadow-free board sprites for the CE scene (task 046).

The scene's `shadows` rows name the boards whose art carries a painted
shadow, with the sprite of that board without the shadow. This sidecar
materializes those sprites for the engine: one row per board, keyed by the
map object's (tile, fid) and the frame and rotation the sprite was built
from, carrying the raw RGBA pixels as the engine's card uploads them. With
the scene's sun shadows on the engine draws these pixels instead of the
board's own art, so the painted shadow does not double the real one.
"""

from __future__ import annotations

import struct

MAGIC = b"F2BS"
VERSION = 1
# magic, version, header bytes, rows
HEADER = struct.Struct("<4sHHH")
# tile, fid, frame, rotation, width, height, ox, oy, then width*height*4 RGBA bytes
ROW = struct.Struct("<iIIBHHhh")


def encode_shadow_sidecar(scene, atlas_images, map_objects) -> bytes:
    """Pack the scene's shadow rows; atlas_images are the packed RGBA arrays."""
    shadows = scene.get("shadows") or []
    boards = scene.get("boards") or []
    sprites = scene.get("sprites") or []
    rows = []
    for index, row in enumerate(shadows):
        if len(row) < 5:
            raise ValueError(f"shadow {index} record is too short")
        board, body, frame, rotation = row[0], row[1], row[3], row[4]
        for value, label in ((board, "board"), (body, "sprite"), (frame, "frame")):
            if isinstance(value, bool) or not isinstance(value, int):
                raise ValueError(f"shadow {index} {label} index must be an integer")
        if not 0 <= board < len(boards):
            raise ValueError(f"shadow {index} names board {board} outside scene.boards")
        if not 0 <= body < len(sprites):
            raise ValueError(f"shadow {index} names sprite {body} outside scene.sprites")
        sprite = sprites[body]
        if len(sprite) < 7:
            raise ValueError(f"shadow {index} sprite record is too short")
        serial = boards[board][5]
        if map_objects is None or isinstance(serial, bool) or not 0 <= serial < len(map_objects):
            continue  # the board's object is not of this elevation
        obj = map_objects[serial]
        image = atlas_images[sprite[0]]
        height, width = image.shape[:2]
        pixels = image.astype("uint8", copy=False).tobytes()
        rows.append(ROW.pack(int(obj.tile), int(obj.fid), int(frame), int(rotation),
                             width, height, int(sprite[5]), int(sprite[6])) + pixels)
    return HEADER.pack(MAGIC, VERSION, HEADER.size, len(rows)) + b"".join(rows)


def decode_shadow_sidecar(data) -> list:
    """Validate and decode the binary contract; shared by tests and tooling."""
    if len(data) < HEADER.size:
        raise ValueError("truncated shadow sidecar header")
    magic, version, header_size, count = HEADER.unpack_from(data)
    if magic != MAGIC or version != VERSION or header_size != HEADER.size:
        raise ValueError("unsupported shadow sidecar magic/version/header")
    offset = header_size
    rows = []
    for _ in range(count):
        if offset + ROW.size > len(data):
            raise ValueError("truncated shadow sidecar row")
        tile, fid, frame, rotation, width, height, ox, oy = ROW.unpack_from(data, offset)
        offset += ROW.size
        if width == 0 or height == 0 or width > 4096 or height > 4096:
            raise ValueError("shadow sidecar sprite size is outside limits")
        size = width * height * 4
        if offset + size > len(data):
            raise ValueError("truncated shadow sidecar pixels")
        rgba = data[offset:offset + size]
        offset += size
        rows.append({"tile": tile, "fid": fid, "frame": frame, "rotation": rotation,
                     "width": width, "height": height, "ox": ox, "oy": oy, "rgba": rgba})
    if offset != len(data):
        raise ValueError("shadow sidecar trailing bytes")
    return rows
