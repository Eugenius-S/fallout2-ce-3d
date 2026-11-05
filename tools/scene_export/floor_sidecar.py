"""Versioned binary floor and roof squares consumed by the CE scene renderer."""

import base64
import struct

from scene_check import one_map_elevation

MAGIC = b"F2FL"
VERSION = 7  # 2: a roof layer follows the floor layer; 3: a roof fringe list follows the cells; 4: the list is wall-top runs; 5: each run names its wall; 6: a list of roof squares with a cut cell of their own follows the runs; 7: the flat water colour (3 bytes RGB) and a byte per square follow the patches: 1 open water, 2 the shore - these squares continue past a map edge (168)
GRID_WIDTH = 100
GRID_HEIGHT = 100
CELL_SIZE = 88
CELL_MARGIN = 6
CELL_OUTER_SIZE = CELL_SIZE + 2 * CELL_MARGIN
HEADER = struct.Struct("<4s8H")
FLOOR_BYTES = GRID_WIDTH * GRID_HEIGHT * 2
FRINGE_ENTRY = struct.Struct("<8H4f")  # wall-top run: edge 0-3, plane, start, count (tiles), its strip rect in roof_fringe.png: x, y, width, height, then its wall: outer face plane (tiles), top (GL height), band span lo, hi along the line (tiles)
PATCH_ENTRY = struct.Struct("<4H")  # a roof square drawn from a cell of roof_fringe.png under the free camera: square index, x, y of the cell, 0


def cell_content_rect(cell, image_size):
    """Return the drawable [left, top, right, bottom) pixels inside one cell."""
    x, y = cell
    image_width, image_height = image_size
    if x < 0 or y < 0 or x + CELL_OUTER_SIZE > image_width or y + CELL_OUTER_SIZE > image_height:
        raise ValueError("tile cell is outside tiles image")
    return (x + CELL_MARGIN, y + CELL_MARGIN,
            x + CELL_MARGIN + CELL_SIZE, y + CELL_MARGIN + CELL_SIZE)


def encode_floor_sidecar(scene, fringe=(), patches=(), water=None, water_color=None):
    """Encode floor and roof atlas indices and tile cells from one format-6 scene.

    fringe: (edge, plane, start, count, x, y, width, height, face, top, lo, hi) of each run of roof
    wall tops (roof_fringe.py): the tile edge (0, 1: along u at v = plane; 2, 3:
    along v at u = plane), the tile line the wall stands on, the first square
    and the number of squares along it, the rect of its strip in roof_fringe.png,
    the plane of the wall's outer face under it, the wall's top height and the
    span the band covers along the line.
    patches: (square index, x, y) of each roof square whose roof under the free camera is the cell at (x, y)
    in roof_fringe.png (its wall-top part cut out, standing as a band).
    water: the 100x100 flags of what continues past a map edge (SceneBuilder.build_water):
    1 open water, 2 the shore line, 0 land or nothing. water_color: the flat
    water colour, 0..1 RGB (SceneBuilder.water_color). Given water, the
    sidecar is version 7 and carries the colour (3 bytes) and the flags (a
    byte per square) after the patches; without it, version 6.
    """
    if scene.get("format") != 6 or not one_map_elevation(scene):
        raise ValueError("floor sidecar expects one map elevation in scene format 6")
    tiles = scene.get("tiles")
    if not isinstance(tiles, dict):
        raise ValueError("scene tiles metadata is missing")
    size = tiles.get("size")
    cells = tiles.get("cells")
    if not isinstance(size, list) or len(size) != 2 or not all(isinstance(v, int) for v in size):
        raise ValueError("invalid tiles image dimensions")
    image_width, image_height = size
    if image_width <= 0 or image_height <= 0 or image_width > 65535 or image_height > 65535:
        raise ValueError("tiles image dimensions are outside sidecar limits")
    # An empty cells list is a tile-less elevation (encfite's void boxes:
    # every square is the nothing tile); a drawn square over it fails below
    # at "floor references a missing tiles.cells entry".
    if not isinstance(cells, list) or len(cells) > 65535:
        raise ValueError("invalid tiles.cells list")

    layers = []
    for layer in ("floor", "roof"):
        try:
            payload = base64.b64decode(scene[layer], validate=True)
        except (KeyError, ValueError, TypeError) as error:
            raise ValueError(f"invalid scene {layer} payload") from error
        if len(payload) != FLOOR_BYTES:
            raise ValueError(f"{layer} payload must be exactly {FLOOR_BYTES} bytes")
        layers.append(payload)
    floor_bytes, roof_bytes = layers
    floor = struct.unpack("<10000H", floor_bytes)
    roof = struct.unpack("<10000H", roof_bytes)

    encoded_cells = bytearray()
    for index, cell in enumerate(cells):
        if not isinstance(cell, list) or len(cell) != 2 or not all(isinstance(v, int) for v in cell):
            raise ValueError(f"invalid tile cell {index}")
        x, y = cell
        try:
            cell_content_rect((x, y), (image_width, image_height))
        except ValueError as error:
            raise ValueError(f"tile cell {index} is outside tiles image") from error
        encoded_cells += struct.pack("<HH", x, y)
    if max(floor) > len(cells) or max(roof) > len(cells):
        raise ValueError("floor references a missing tiles.cells entry")

    trailer = struct.pack("<H", len(fringe))
    for edge, plane, start, count, x, y, width, height, *wall in fringe:
        if edge > 3 or plane > GRID_WIDTH or count == 0 or start + count > GRID_WIDTH or width == 0 or height == 0:
            raise ValueError("roof fringe entry is outside the map or empty")
        trailer += FRINGE_ENTRY.pack(edge, plane, start, count, x, y, width, height, *wall)
    trailer += struct.pack("<H", len(patches))
    for index, x, y in patches:
        if not 0 <= index < GRID_WIDTH * GRID_HEIGHT or not roof[index]:
            raise ValueError("roof patch entry is not a roof square")
        trailer += PATCH_ENTRY.pack(index, x, y, 0)
    if water is not None:
        flags = bytes(water)
        if len(flags) != GRID_WIDTH * GRID_HEIGHT or any(f not in (0, 1, 2) for f in flags):
            raise ValueError("water flags must be one 0, 1 or 2 byte per square")
        colour = water_color
        if not isinstance(colour, (list, tuple)) or len(colour) != 3 or any(not isinstance(v, (int, float)) or not 0 <= v <= 1 for v in colour):
            raise ValueError("water colour must be three 0..1 floats")
        trailer += bytes(round(v * 255) for v in colour) + flags
    version = VERSION if water is not None else 6
    header = HEADER.pack(MAGIC, version, GRID_WIDTH, GRID_HEIGHT, len(cells),
                         image_width, image_height, CELL_SIZE, CELL_MARGIN)
    return header + floor_bytes + roof_bytes + bytes(encoded_cells) + trailer


def decode_floor_sidecar(data):
    """Decode and validate the sidecar; used by format regression tests."""
    if len(data) < HEADER.size + 2 * FLOOR_BYTES:
        raise ValueError("truncated floor sidecar")
    magic, version, grid_w, grid_h, tile_count, image_w, image_h, cell_size, margin = HEADER.unpack_from(data)
    if magic != MAGIC or version not in (5, 6, VERSION):
        raise ValueError("unsupported floor sidecar magic/version")
    if (grid_w, grid_h, cell_size, margin) != (GRID_WIDTH, GRID_HEIGHT, CELL_SIZE, CELL_MARGIN):
        raise ValueError("unsupported floor sidecar geometry")
    cells_end = HEADER.size + 2 * FLOOR_BYTES + tile_count * 4
    if len(data) < cells_end + 2:
        raise ValueError("truncated floor sidecar")
    fringe_count = struct.unpack_from("<H", data, cells_end)[0]
    patches_at = cells_end + 2 + fringe_count * FRINGE_ENTRY.size
    if version >= 6:
        if len(data) < patches_at + 2:
            raise ValueError("truncated floor sidecar")
        patch_count = struct.unpack_from("<H", data, patches_at)[0]
        body_end = patches_at + 2 + patch_count * PATCH_ENTRY.size
    else:
        patch_count = 0
        body_end = patches_at
    expected = body_end + ((3 + GRID_WIDTH * GRID_HEIGHT) if version >= 7 else 0)
    if len(data) < expected:
        raise ValueError("truncated floor sidecar")
    if len(data) != expected or image_w == 0 or image_h == 0:
        raise ValueError("invalid floor sidecar length or dimensions")
    floor_start = HEADER.size
    roof_start = floor_start + FLOOR_BYTES
    cells_start = roof_start + FLOOR_BYTES
    floor = struct.unpack_from("<10000H", data, floor_start)
    roof = struct.unpack_from("<10000H", data, roof_start)
    cells = [struct.unpack_from("<HH", data, cells_start + i * 4) for i in range(tile_count)]
    if max(floor) > tile_count or max(roof) > tile_count:
        raise ValueError("floor references a missing tiles.cells entry")
    for x, y in cells:
        if x + cell_size + 2 * margin > image_w or y + cell_size + 2 * margin > image_h:
            raise ValueError("tile cell is outside tiles image")
    fringe = [FRINGE_ENTRY.unpack_from(data, cells_end + 2 + i * FRINGE_ENTRY.size) for i in range(fringe_count)]
    for edge, plane, start, count, x, y, width, height, *_ in fringe:
        if edge > 3 or plane > grid_w or count == 0 or start + count > grid_w or width == 0 or height == 0:
            raise ValueError("roof fringe entry is outside the map or empty")
    patches = [PATCH_ENTRY.unpack_from(data, patches_at + 2 + i * PATCH_ENTRY.size)[:3] for i in range(patch_count)]
    water = list(data[body_end + 3:body_end + 3 + GRID_WIDTH * GRID_HEIGHT]) if version >= 7 else None
    colour = [v / 255 for v in data[body_end:body_end + 3]] if version >= 7 else None
    if water is not None and any(f not in (0, 1, 2) for f in water):
        raise ValueError("water flags must be one 0, 1 or 2 byte per square")
    return {
        "version": version,
        "fringe": fringe,
        "patches": patches,
        "water": water,
        "waterColor": colour,
        "grid_size": (grid_w, grid_h),
        "image_size": (image_w, image_h),
        "cell_size": cell_size,
        "cell_margin": margin,
        "floor": floor,
        "roof": roof,
        "cells": cells,
    }
