"""Task 120 slice 2 validation: carve a heightfield from an art's six
rotation frames and see whether it reads as the thing.

    py -3 tools/shapes/carve120.py car4b [chirality]

The game camera (proj.SQ_U/SQ_V/HPX) at six azimuths 60 degrees apart is
the world rotated about the object's anchor hex by the hex rotation
(u, v) -> (-v, u+v) (chirality 0) or (u+v, -u) (chirality 1). A world cell
survives at a height when every frame's silhouette is opaque at the cell's
projected pixel (the anchor inside each frame is frm.bounds's:
w // 2 - x_off, h - 1 - y_off). The wrong chirality kills the upper body,
so the surviving count above half height picks it.

Writes build/_sweep120/carve-<name>.png: the d0 silhouette, then the carved
columns' max height as grayscale (bright = tall), and prints the height
profile.
"""
from __future__ import annotations

import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "scene_export"))
sys.path.insert(0, str(ROOT / "tools" / "scene_export" / "vendor"))

from export import ReadOnlyGameFiles  # noqa: E402
from f2lib.frm import Frm  # noqa: E402
from fp import proj  # noqa: E402

GAME = ROOT / "game"
OUT = ROOT / "build" / "_sweep120"
CELL = 1.0 / proj.BB_PX               # world units per footprint px step
ROTS = (((0, -1), (1, 1)),           # chirality 0: (u, v) -> (-v, u+v)
        ((1, 1), (0, -1)))           # chirality 1: (u, v) -> (u+v, -u)


def silhouettes(gf, name):
    """Per direction: the opaque art without its painted floor shadow
    (light.outer_dark on the palette indices), and the frm.bounds anchor."""
    from fp import light
    arts = gf.art_list(2)
    index = arts.index(name.lower() + ".frm")
    frm = Frm.from_bytes(gf.read(f"art/scenery/{arts.names[index]}"))
    sils, raws, anchors = [], [], []
    for d in range(6):
        idx = frm.frame(d, 0).array()
        rgba = frm.rgba(gf.palette, d, 0)
        sils.append((idx != 0) & ~light.outer_dark(idx))
        raws.append(rgba[..., 3] != 0)
        w, h = rgba.shape[1], rgba.shape[0]
        anchors.append((w // 2 - int(frm.x_offsets[d]), (h - 1) - int(frm.y_offsets[d])))
    return sils, raws, anchors


def rot(d, chirality):
    """The hex rotation's (a, b, c, e): (u, v) -> (a*u + b*v, c*u + e*v)."""
    if d == 0:
        return 1, 0, 0, 1
    a, b = ROTS[chirality][0]
    c, e = ROTS[chirality][1]
    m = np.array([[a, b], [c, e]])
    r = np.linalg.matrix_power(m, d)
    return r[0, 0], r[0, 1], r[1, 0], r[1, 1]


def carve(sils, anchors, chirality, r=110, ztop=72, shifts=None, step=1, zstep=1):
    """The boolean hull volume (u, v, z), footprint px steps, height in frame px.

    A sample outside its frame is sky: empty, never a hit. `shifts`: per
    direction (dx, dy) px registration correction on the anchor. `step`/
    `zstep` thin the grid (registration searches on a coarse one)."""
    if shifts is None:
        shifts = [(0, 0)] * 6
    steps = np.arange(-r, r + 1, step) * CELL
    zs = np.arange(0, ztop, zstep)
    u = np.broadcast_to(steps[:, None], (len(steps), len(steps)))
    v = np.broadcast_to(steps[None, :], (len(steps), len(steps)))
    volume = np.ones(u.shape + (len(zs),), bool)
    for d in range(6):
        a, b, c, e = rot(d, chirality)
        ru, rv = a * u + b * v, c * u + e * v
        w, h = sils[d].shape[1], sils[d].shape[0]
        ax, ay = anchors[d][0] + shifts[d][0], anchors[d][1] + shifts[d][1]
        sx = np.round(ru * proj.SQ_U[0] + rv * proj.SQ_V[0] + ax).astype(int)
        sy = np.round(ru * proj.SQ_U[1] + rv * proj.SQ_V[1] + ay).astype(int)
        yi = sy[..., None] - zs[None, None, :]
        xi = np.broadcast_to(sx[..., None], yi.shape)
        inside = (xi >= 0) & (xi < w) & (yi >= 0) & (yi < h)
        volume &= inside & sils[d][np.clip(yi, 0, h - 1), np.clip(xi, 0, w - 1)]
    return volume, steps, zs


def register(sils, anchors, chirality, reach=20):
    """Coordinate descent on the frames' (dx, dy): the misalignment of any
    frame only shrinks the six-way intersection, so the shift set that
    maximizes the surviving voxels is the registration. Direction 0 stays
    put (it fixes the hull's gauge). A coarse pass (grid step 4, z step 2)
    sweeps wide, a fine pass (step 1) polishes within 4 px of it."""
    shifts = [(0, 0)] * 6

    def score(sh, coarse):
        volume, _, _ = carve(sils, anchors, chirality, r=84, ztop=48, shifts=sh,
                             step=4 if coarse else 1, zstep=2 if coarse else 1)
        return int(volume.sum())

    best = score(shifts, True)
    for rnd in range(3):
        moved = False
        for d in range(1, 6):
            bx, by = shifts[d]
            for dx in range(-reach, reach + 1, 4):
                for dy in range(-reach, reach + 1, 4):
                    trial = list(shifts)
                    trial[d] = (dx, dy)
                    s = score(trial, True)
                    if s > best:
                        best, bx, by, moved = s, dx, dy, True
            shifts[d] = (bx, by)
        if not moved:
            break
    for rnd in range(2):
        moved = False
        for d in range(1, 6):
            bx, by = shifts[d]
            for dx in range(bx - 4, bx + 5):
                for dy in range(by - 4, by + 5):
                    trial = list(shifts)
                    trial[d] = (dx, dy)
                    s = score(trial, False)
                    if s > best:
                        best, bx, by, moved = s, dx, dy, True
            shifts[d] = (bx, by)
        if not moved:
            break
    return shifts, best


def main():
    name = sys.argv[1] if len(sys.argv) > 1 else "car4b"
    force = int(sys.argv[2]) if len(sys.argv) > 2 else -1
    mode = sys.argv[3] if len(sys.argv) > 3 else "hdr"
    OUT.mkdir(parents=True, exist_ok=True)
    gf = ReadOnlyGameFiles(GAME)
    try:
        sils, raws, hdr = silhouettes(gf, name)
    finally:
        gf.close()
    if mode == "hdr":                       # frm.bounds's convention (the exporter's)
        anchors = hdr
    else:                                   # bottom-centre, what the engine's turn path uses
        anchors = [(s.shape[1] // 2, s.shape[0] - 1) for s in sils]
    print(f"mode {mode} anchors:", anchors)
    print("silhouette sizes:", [s.shape for s in sils],
          "shadow-free px:", [int(s.sum()) for s in sils])

    best, best_state = (force if force >= 0 else None), None
    for chirality in ((force,) if force >= 0 else (0, 1)):
        shifts, s = register(sils, anchors, chirality)
        print(f"chirality {chirality}: registered shifts {shifts}, coarse voxels {s}")
        volume, steps, zs = carve(sils, anchors, chirality, shifts=shifts)
        state = (int(volume.sum()), chirality, shifts, volume, steps, zs)
        if best_state is None or state[0] > best_state[0]:
            best, best_state = chirality, state
    print(f"best chirality: {best}")
    total, chirality, shifts, volume, steps, zs = best_state
    hmap = np.where(volume.any(axis=2), volume.argmax(axis=2), 0).astype(np.uint8)
    img = Image.fromarray(255 - hmap * (255 // max(1, len(zs))), "L")

    d0 = raws[0]
    h, w = d0.shape
    canvas = Image.new("L", (w + img.width + 12, max(h, img.height)), 40)
    canvas.paste(Image.fromarray((d0 * 255).astype(np.uint8), "L"), (0, 0))
    canvas.paste(img, (w + 12, 0))
    out = OUT / f"carve-{name}.png"
    canvas.save(out)
    print("wrote", out)

    columns = volume.any(axis=2)
    for z in (7, 21, 43, 64):
        if z < len(zs):
            print(f"columns alive at z={z:3d}px: {int(volume[..., z].sum())}")
    print(f"footprint columns: {int(columns.sum())}, "
          f"mean height {volume.sum() / max(1, columns.sum()):.1f} px")

    # registration health: the hull projected back into each frame must
    # cover a good share of that frame's silhouette
    steps, u, v = steps, None, None
    uu = np.broadcast_to(steps[:, None], (len(steps), len(steps)))
    vv = np.broadcast_to(steps[None, :], (len(steps), len(steps)))
    top = volume.argmax(axis=2)
    has = volume.any(axis=2)
    for d in range(6):
        a, b, c, e = rot(d, chirality)
        ru, rv = a * uu + b * vv, c * uu + e * vv
        w, h = sils[d].shape[1], sils[d].shape[0]
        ax, ay = anchors[d][0] + shifts[d][0], anchors[d][1] + shifts[d][1]
        sx = np.round(ru * proj.SQ_U[0] + rv * proj.SQ_V[0] + ax).astype(int)
        sy = np.round(ru * proj.SQ_U[1] + rv * proj.SQ_V[1] + ay).astype(int)
        cover = np.zeros(sils[d].shape, bool)
        ok = has & (sx >= 0) & (sx < w) & (sy >= 0) & (sy < h)
        cover[np.clip(sy[ok], 0, h - 1), np.clip(sx[ok], 0, w - 1)] = True
        # also the faces: every column covers from z=0 to its top
        for z in range(1, int(top[ok].max()) + 1 if ok.any() else 1):
            yy = sy[ok] - z
            m = (yy >= 0) & (yy < h)
            cover[np.clip(yy[m], 0, h - 1), np.clip(sx[ok][m], 0, w - 1)] = True
        inter = int((cover & sils[d]).sum())
        union = int((cover | sils[d]).sum())
        print(f"d{d}: hull|art IoU {inter / max(1, union):.2f} "
              f"(art px {int(sils[d].sum())}, hull covers {int(cover.sum())})")


if __name__ == "__main__":
    main()
