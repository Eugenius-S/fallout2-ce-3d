"""Compare two walls.bin files: game-drawn batches (flags 0) vs fly-only (1/2)."""
import struct
import sys

HEADER = struct.Struct("<4sHHIIII")
BATCH = struct.Struct("<IIIII")
FLOATS = 9

def batches(path):
    data = open(path, "rb").read()
    magic, version, header_size, pages, batch_count, triangles, objects = HEADER.unpack_from(data)
    offset = header_size
    out = []
    for _ in range(batch_count):
        page, width, height, count, flags = BATCH.unpack_from(data, offset)
        offset += BATCH.size
        raw = data[offset:offset + count * 3 * FLOATS * 4]
        offset += len(raw)
        out.append((flags, page, raw))
    return out

for map_, elev in (("klamall", 0), ("kladwtwn", 0), ("klatoxcv", 0), ("klatoxcv", 2), ("klaratcv", 1)):
    base = batches(f"build/scene-cache/{map_}/{elev}/walls.bin")
    fix = batches(f"build/cache-119-fix/{map_}/{elev}/walls.bin")
    gb, gf = sorted(x for x in base if x[0] == 0), sorted(x for x in fix if x[0] == 0)
    fb, ff = sorted(x for x in base if x[0]), sorted(x for x in fix if x[0])
    new_fly = [x for x in ff if x not in fb]
    print(f"{map_} e{elev}: game batches {len(gb)} vs {len(gf)} identical: {gb == gf}; "
          f"fly batches {len(fb)} vs {len(ff)}, new fly batches: {len(new_fly)}, "
          f"new triangles: {sum(x[2].count(b'') and len(x[2]) // (3 * FLOATS * 4) for x in new_fly)}")
