"""Versioned binary floor and roof squares consumed by the CE scene renderer."""

import base64
import struct

from scene_check import one_map_elevation

MAGIC = b"F2FL"
VERSION = 2  # 2: a roof layer follows the floor layer
GRID_WIDTH = 100
GRID_HEIGHT = 100
CELL_SIZE = 88
CELL_MARGIN = 6
CELL_OUTER_SIZE = CELL_SIZE + 2 * CELL_MARGIN
HEADER = struct.Struct("<4s8H")
FLOOR_BYTES = GRID_WIDTH * GRID_HEIGHT * 2


def cell_content_rect(cell, image_size):
    """Return the drawable [left, top, right, bottom) pixels inside one cell."""
    x, y = cell
    image_width, image_height = image_size
    if x < 0 or y < 0 or x + CELL_OUTER_SIZE > image_width or y + CELL_OUTER_SIZE > image_height:
        raise ValueError("tile cell is outside tiles image")
    return (x + CELL_MARGIN, y + CELL_MARGIN,
            x + CELL_MARGIN + CELL_SIZE, y + CELL_MARGIN + CELL_SIZE)


def encode_floor_sidecar(scene):
    """Encode floor and roof atlas indices and tile cells from one format-6 scene."""
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
    if not isinstance(cells, list) or not cells or len(cells) > 65535:
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

    header = HEADER.pack(MAGIC, VERSION, GRID_WIDTH, GRID_HEIGHT, len(cells),
                         image_width, image_height, CELL_SIZE, CELL_MARGIN)
    return header + floor_bytes + roof_bytes + bytes(encoded_cells)


def decode_floor_sidecar(data):
    """Decode and validate the sidecar; used by format regression tests."""
    if len(data) < HEADER.size + 2 * FLOOR_BYTES:
        raise ValueError("truncated floor sidecar")
    magic, version, grid_w, grid_h, tile_count, image_w, image_h, cell_size, margin = HEADER.unpack_from(data)
    if magic != MAGIC or version != VERSION:
        raise ValueError("unsupported floor sidecar magic/version")
    if (grid_w, grid_h, cell_size, margin) != (GRID_WIDTH, GRID_HEIGHT, CELL_SIZE, CELL_MARGIN):
        raise ValueError("unsupported floor sidecar geometry")
    expected = HEADER.size + 2 * FLOOR_BYTES + tile_count * 4
    if len(data) < expected:
        raise ValueError("truncated floor sidecar")
    if len(data) != expected or tile_count == 0 or image_w == 0 or image_h == 0:
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
    return {
        "version": version,
        "grid_size": (grid_w, grid_h),
        "image_size": (image_w, image_h),
        "cell_size": cell_size,
        "cell_margin": margin,
        "floor": floor,
        "roof": roof,
        "cells": cells,
    }
