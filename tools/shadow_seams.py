"""The seams in the sun's shadow, 090: 0 only when there are none.

Reads a -ShadowDebug:1 snapshot (the raw factor, 0 lit, 1 fully in shadow,
flat gray; what takes no sun shadow - roofs, the interface - reads magenta
and is left out, so its outline against the ground is not a seam). The
shadow class is the factor at 0.5 and up. Inside the
central box (past the picture's border and the interface) it counts:

- holes: lit pixels the shadow should hold - the lit components of at most
  --area pixels with shadow all round, and the lit lines at most 2 pixels
  wide that a 3 x 3 closing of the shadow fills (a tile joint, a crack);
- specks: shadowed components of at most --area pixels with light all
  round (a lone dot, a star);
- dips: inside the shadow, past --edge pixels from its outline (the AA
  ring), pixels that do not read the full factor (a half-lit line or dot);
- grains: inside the light, past --edge pixels from the outline, pixels that
  read some shadow.

Every count is in picture pixels; the total is 0 only on a seamless view.
An overlay PNG marks what it counted: red holes, yellow dips, blue specks,
cyan grains, over the factor dimmed.

py -3 tools/shadow_seams.py <debug1.png> [--overlay out.png] [--area 12]
    [--edge 4] [--box x0,y0,x1,y1]
"""

import argparse
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage


def factor(path):
    """The factor, and where the picture shows no receiver (magenta)."""
    with Image.open(path) as image:
        rgb = np.asarray(image.convert("RGB"), dtype=np.float32) / 255.0
    ignore = (rgb[..., 0] > 0.9) & (rgb[..., 1] < 0.1) & (rgb[..., 2] > 0.9)
    return 0.299 * rgb[..., 0] + 0.587 * rgb[..., 1] + 0.114 * rgb[..., 2], ignore


def small_enclosed(mask, area, near_ignore):
    """Components of mask (8-connected) of at most `area` pixels that touch
    neither the box border nor a left-out pixel."""
    labels, count = ndimage.label(mask, structure=np.ones((3, 3), bool))
    if count == 0:
        return np.zeros_like(mask)
    sizes = ndimage.sum_labels(mask, labels, index=np.arange(1, count + 1))
    border = np.unique(np.concatenate([labels[0], labels[-1], labels[:, 0], labels[:, -1],
                                       labels[near_ignore]]))
    keep = sizes <= area
    keep[border[border > 0] - 1] = False
    return keep[labels - 1] & mask


def measure(fac, ignore, area, edge):
    square = np.ones((3, 3), bool)
    near_ignore = ndimage.binary_dilation(ignore, structure=square, iterations=2)
    shadow = (fac >= 0.5) & ~ignore
    lit = (fac < 0.5) & ~ignore
    holes = small_enclosed(lit, area, near_ignore)
    # Lit lines at most 2 px wide closed by the shadow round them: what a
    # 3 x 3 closing of the shadow fills, the picture border left out.
    closed = ndimage.binary_closing(shadow, structure=square, border_value=0)
    holes |= closed & lit & ~near_ignore
    specks = small_enclosed(shadow, area, near_ignore)
    disk = ndimage.generate_binary_structure(2, 1)
    inner_shadow = ndimage.binary_erosion(shadow | ignore, structure=disk, iterations=edge, border_value=1) & shadow
    inner_lit = ndimage.binary_erosion(lit | ignore, structure=disk, iterations=edge, border_value=1) & lit
    inner_shadow &= ~near_ignore
    inner_lit &= ~near_ignore
    dips = inner_shadow & (fac < 0.97) & ~holes
    grains = inner_lit & (fac > 0.03) & ~specks
    return {"holes": holes, "specks": specks, "dips": dips, "grains": grains}


def overlay(fac, found, path):
    base = (fac * 0.45)[..., None].repeat(3, axis=2)
    colors = {"holes": (1, 0, 0), "dips": (1, 1, 0), "specks": (0, 0.4, 1), "grains": (0, 1, 1)}
    for name, color in colors.items():
        base[found[name]] = color
    Image.fromarray((base * 255).astype(np.uint8)).save(path)


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("factor")
    parser.add_argument("--overlay")
    parser.add_argument("--area", type=int, default=12)
    parser.add_argument("--edge", type=int, default=4)
    parser.add_argument("--box")
    args = parser.parse_args(argv[1:])
    fac, ignore = factor(args.factor)
    height, width = fac.shape
    if args.box:
        x0, y0, x1, y1 = (int(v) for v in args.box.split(","))
    else:
        # The central picture, past the border and the interface bars.
        x0, y0, x1, y1 = width // 16, height // 16, width * 15 // 16, height * 7 // 8
    found = measure(fac[y0:y1, x0:x1], ignore[y0:y1, x0:x1], args.area, args.edge)
    counts = {name: int(mask.sum()) for name, mask in found.items()}
    counts["total"] = sum(counts.values())
    if args.overlay:
        full = {name: np.zeros_like(fac, dtype=bool) for name in found}
        for name, mask in found.items():
            full[name][y0:y1, x0:x1] = mask
        overlay(fac, full, args.overlay)
    verdict = "PASS" if counts["total"] == 0 else "FAIL"
    print(f"shadow_seams: {verdict} holes {counts['holes']}, specks {counts['specks']}, "
          f"dips {counts['dips']}, grains {counts['grains']}, {Path(args.factor).name}")
    print(json.dumps({"seams": counts, "box": [x0, y0, x1, y1]}))
    return 0 if verdict == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
