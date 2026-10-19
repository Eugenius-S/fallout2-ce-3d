"""Versioned projection metadata and the donor's live-card coordinate math."""

from __future__ import annotations

import math
import struct

from scene_check import one_map_elevation


MAGIC = b"F2PJ"
VERSION = 1
HEADER = struct.Struct("<4sHHI")  # magic, version, header bytes, float64 count
VALUES = struct.Struct("<10d")  # squ[2], sqv[2], origin[2], hpx, bbPx, rowTilt, stack
MAP_OFFSET = 50.0


def _finite(value, name):
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
        raise ValueError(f"projection {name} must be finite")
    return float(value)


def _projection(scene):
    if not one_map_elevation(scene):
        raise ValueError("projection sidecar requires one map elevation")
    raw = scene.get("proj")
    if not isinstance(raw, dict):
        raise ValueError("scene.proj is missing")
    squ, sqv, origin = raw.get("squ"), raw.get("sqv"), raw.get("origin")
    if not all(isinstance(value, (list, tuple)) and len(value) == 2 for value in (squ, sqv, origin)):
        raise ValueError("scene.proj vectors must each contain two values")
    values = tuple(_finite(value, "vector component") for pair in (squ, sqv, origin) for value in pair) + tuple(
        _finite(raw.get(key), key) for key in ("hpx", "bbPx", "rowTilt", "stack")
    )
    squ_x, squ_y, sqv_x, sqv_y, origin_x, origin_y, hpx, bbpx, row_tilt, stack = values
    determinant = squ_x * sqv_y - sqv_x * squ_y
    if abs(determinant) < 1e-9:
        raise ValueError("scene.proj is not invertible")
    if any(abs(value) > 1000000 for value in values[:6]):
        raise ValueError("scene.proj coordinates exceed safe bounds")
    if not 1e-3 <= hpx <= 4096 or not 1e-3 <= bbpx <= 4096:
        raise ValueError("scene.proj height/width scale is out of bounds")
    if not 0 <= row_tilt <= 1 or not 0 <= stack <= 1:
        raise ValueError("scene.proj depth values are out of bounds")
    return values


def encode_projection_sidecar(scene):
    values = _projection(scene)
    return HEADER.pack(MAGIC, VERSION, HEADER.size, len(values)) + VALUES.pack(*values)


def decode_projection_sidecar(payload):
    if len(payload) != HEADER.size + VALUES.size:
        raise ValueError("projection sidecar has a bad length")
    magic, version, header_size, value_count = HEADER.unpack_from(payload)
    if magic != MAGIC or version != VERSION or header_size != HEADER.size or value_count != 10:
        raise ValueError("projection sidecar magic/version/header is unsupported")
    values = VALUES.unpack_from(payload, HEADER.size)
    # Run the same numeric bounds checks as the encoder using a canonical scene shape.
    keys = ("hpx", "bbPx", "rowTilt", "stack")
    scene = {"map": "kladwtwn", "elevation": 0, "proj": {
        "squ": values[:2], "sqv": values[2:4], "origin": values[4:6],
        **dict(zip(keys, values[6:])),
    }}
    validated = _projection(scene)
    return dict(squ=validated[:2], sqv=validated[2:4], origin=validated[4:6],
                hpx=validated[6], bbPx=validated[7], rowTilt=validated[8], stack=validated[9])


def delta_to_ground(projection, dx, dy):
    squ, sqv = projection["squ"], projection["sqv"]
    determinant = squ[0] * sqv[1] - sqv[0] * squ[1]
    return ((sqv[1] * dx - sqv[0] * dy) / determinant,
            (squ[0] * dy - squ[1] * dx) / determinant)


def hex_pixel(tile):
    hx, hy = tile % 200, tile // 200
    a = 199 - hx
    return (48 * (a >> 1) + 32 * (a & 1) + 16 * hy + 16,
            -12 * (a >> 1) + 12 * hy + 8)


def hero_anchor(projection, tile, object_x=0, object_y=0):
    """Scene world (u-50,v-50) with object pixel movement applied exactly once."""
    px, py = hex_pixel(tile)
    origin = projection["origin"]
    du, dv = delta_to_ground(projection, px + object_x - origin[0], py + object_y - origin[1])
    return du - MAP_OFFSET, dv - MAP_OFFSET


def standing_card(projection, width, height, art_x_offset, art_y_offset):
    """Donor upright-card offsets; Object.x/y belong to hero_anchor, not here."""
    if width <= 0 or height <= 0:
        raise ValueError("hero frame dimensions must be positive")
    top = art_y_offset - (height - 1)
    bottom = top + height
    if bottom <= 0:
        up0, up1 = -bottom / projection["hpx"], -top / projection["hpx"]
    else:
        far = min(bottom, max(0, top))
        up0 = 0.0
        up1 = (far - top + 0.5 * (bottom - far)) / projection["hpx"]
    left_pixels = art_x_offset - width // 2
    right_pixels = left_pixels + width
    return left_pixels / projection["bbPx"], right_pixels / projection["bbPx"], up0, up1
