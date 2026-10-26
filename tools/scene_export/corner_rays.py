"""Holes at wall corners for the free camera (task 076).

Usage: py -3 tools/scene_export/corner_rays.py <scene dir> [...]
  e.g. build/scene-cache/kladwtwn/0

For every L corner of thick walls, horizontal rays from 18 sides, across
the corner and at three heights, that cross where the walls stand must
meet an opaque texel of walls.bin's free-camera geometry. Prints the
corners whose rays go through, worst first. A mesh fence is clear on
purpose, so its corners show up here too.
"""
import json
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image

HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(HERE), str(HERE / "vendor")]
from solids import Projection  # noqa: E402
from wall_sidecar import _corner_fits, decode_wall_sidecar  # noqa: E402
from f2lib.geometry import hex_from_world  # noqa: E402


def check(base):
    scene = json.loads((base / "scene.json").read_text(encoding="utf-8"))
    walls, proj = scene["walls"], Projection(scene["proj"])
    corners = _corner_fits(walls, set(scene.get("hiddenWalls") or []), proj.toward)
    tris, uvs, pages = [], [], []
    for batch in decode_wall_sidecar((base / "walls.bin").read_bytes())["batches"]:
        if batch["game_only"]:
            continue
        vs = batch["vertices"]
        for i in range(0, len(vs), 3):
            tris.append([(x + 50, z + 50, y) for x, y, z, *_ in vs[i:i + 3]])
            uvs.append([(u, v) for _, _, _, u, v, *_ in vs[i:i + 3]])
            pages.append(batch["page"])
    if not tris:
        return []
    tri, uv, centre = np.array(tris), np.array(uvs), np.array(tris).mean(axis=1)
    alpha = {}

    def opaque(page, u, v):
        if page not in alpha:
            alpha[page] = np.array(Image.open(base / f"sprites{page}.png").convert("RGBA"))[:, :, 3]  # a page may be a palette PNG
        a = alpha[page]
        return a[min(int(v * a.shape[0]), a.shape[0] - 1), min(int(u * a.shape[1]), a.shape[1] - 1)] >= 128

    ends = {(id(slab), out): target for slab, out, target, _ in corners.targets}

    def footprint(slab):
        lo, hi = ends.get((id(slab), -1), slab.lo), ends.get((id(slab), 1), slab.hi)
        return ((lo, hi), slab.band()) if slab.axis == 0 else (slab.band(), (lo, hi))

    found = []
    for slab, _, _, other in corners.targets:
        if other is None or slab.thick < 0.2:
            continue
        rects = [footprint(slab), footprint(other)]
        (u0, u1), (v0, v1) = rects[0][0], rects[0][1]
        cu, cv = (sum(other.band()) / 2, sum(slab.band()) / 2) if slab.axis == 0 else \
            (sum(slab.band()) / 2, sum(other.band()) / 2)
        top = walls[slab.rows[0]][9]
        near = np.where(np.hypot(centre[:, 0] - cu, centre[:, 1] - cv) < 2.0)[0]
        v0_, e1, e2 = tri[near, 0], tri[near, 1] - tri[near, 0], tri[near, 2] - tri[near, 0]

        def crosses(o, d):
            for (a0, a1), (b0, b1) in rects:
                t0, t1 = 0.0, 3.0
                for k, (lo, hi) in enumerate(((a0, a1), (b0, b1))):
                    if abs(d[k]) < 1e-9:
                        if not lo + 0.02 < o[k] < hi - 0.02:
                            t0, t1 = 1, 0
                        continue
                    ta, tb = sorted(((lo - o[k]) / d[k], (hi - o[k]) / d[k]))
                    t0, t1 = max(t0, ta), min(t1, tb)
                if t1 - t0 > 0.03:
                    return True
            return False

        rays = through = 0
        for az in range(0, 360, 20):
            d = np.array([-math.cos(math.radians(az)), -math.sin(math.radians(az)), 0.0])
            side = np.array([-d[1], d[0], 0.0])
            for off in np.linspace(-0.35, 0.35, 15):
                for z in (0.3, 0.9, 1.5):
                    o = np.array([cu, cv, z]) - 1.5 * d + off * side
                    if z > top - 0.05 or not crosses(o, d):
                        continue
                    rays += 1
                    pv = np.cross(d, e2)
                    det = np.einsum("ij,ij->i", e1, pv)
                    safe = np.where(np.abs(det) > 1e-12, det, 1.0)
                    tv = o - v0_
                    uu = np.einsum("ij,ij->i", tv, pv) / safe
                    qv = np.cross(tv, e1)
                    vv = (qv @ d) / safe
                    tt = np.einsum("ij,ij->i", e2, qv) / safe
                    hit = (np.abs(det) > 1e-12) & (uu >= 0) & (vv >= 0) & (uu + vv <= 1) & (tt > 0) & (tt < 3)
                    if not any(opaque(pages[near[j]], *(uv[near[j]][0] * (1 - uu[j] - vv[j]) + uv[near[j]][1] * uu[j]
                                                       + uv[near[j]][2] * vv[j]))
                               for j in np.where(hit)[0][np.argsort(tt[hit])]):
                        through += 1
        if through:
            x, y = proj.ground_to_px(cu, cv)
            found.append((through, rays, hex_from_world(int(x), int(y))))
    return sorted({tile: (through, rays, tile) for through, rays, tile in found}.values(), reverse=True)


def main(paths):
    for path in paths:
        found = check(Path(path))
        print(f"{path}: {len(found)} corners with rays through")
        for through, rays, tile in found:
            print(f"  tile {tile}: {through} of {rays}")


if __name__ == "__main__":
    main(sys.argv[1:])
