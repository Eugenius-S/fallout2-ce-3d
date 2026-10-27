"""The stripes and the edge of the sun's shadow, 082.

Stripes: over the faces the sun does not hit, the factor must be flat. The
faces come from a -ShadowDebug:2 snapshot (white the sun hits, black a face
in its own shadow), the factor from a -ShadowDebug:1 snapshot of the same
view (the background runs are byte-stable, the two match pixel for pixel).
Per column, the mean factor over that column's face pixels; the stripes are
how much that mean steps from one column to the next. Stripes would step
often; a real shadow edge steps once. The script reports the largest step,
how many steps pass 0.05, and the longest run of steps that keep passing it.

Edge: across a ground shadow's straight edge the factor sweeps 0 to 1 over a
few pixels. The script walks the central part of the picture, finds the
sharpest dark-to-bright climb along a row, and reports the width of its
10%..90% transition in game pixels (from the snapshot's scale).

py -3 tools/shadow_stripes.py <debug1.png> <debug2.png>
"""

import json
import sys
from pathlib import Path

from PIL import Image, ImageFilter


def gray(path, erode=0):
    with Image.open(path) as image:
        rgb = image.convert("RGB")
        if erode:
            # Erode the black face mask: only keep a pixel whose whole
            # neighbourhood is black too (MaxFilter takes the brightest
            # neighbour, so black regions shrink by the radius). The picture's
            # border and the interface read black in the face-test debug
            # without being faces; erosion sheds them.
            rgb = rgb.filter(ImageFilter.MaxFilter(erode))
        data = list(rgb.getdata())
        g = [(0.299 * r + 0.587 * g + 0.114 * b) / 255.0 for r, g, b in data]
        return g, rgb.size[0], rgb.size[1]


def stripes(fac, mask, width, height, box, min_pixels=8, threshold=0.05):
    """Per-column mean factor over the face mask, and how it steps. The box
    (x0, x1, y0, y1) keeps the central picture, past border and interface.
    Mask pixels that read lit-black too are the void past the map's edge, not
    faces (a face in its own shadow is the flat shaded value, never 0)."""
    x0, x1, y0, y1 = box
    sums = [0.0] * width
    counts = [0] * width
    for y in range(y0, y1):
        row = y * width
        for x in range(x0, x1):
            if mask[row + x] < 0.5 and fac[row + x] >= 0.02:
                sums[x] += fac[row + x]
                counts[x] += 1
    means = [sums[x] / counts[x] if counts[x] >= min_pixels else None for x in range(width)]
    steps = []
    for x in range(width - 1):
        a, b = means[x], means[x + 1]
        steps.append(None if a is None or b is None else abs(b - a))
    known = [s for s in steps if s is not None]
    if not known:
        return None
    worst = max(known)
    over = sum(1 for s in known if s > threshold)
    # The longest run of neighbouring steps that all pass the threshold: a
    # stripe field runs long, a real edge is 1-2 steps wide.
    longest = 0
    run = 0
    for s in known:
        run = run + 1 if s > threshold else 0
        longest = max(longest, run)
    columns = sum(1 for c in counts if c >= min_pixels)
    return {
        "face_columns": columns,
        "worst_column_step": round(worst, 4),
        "steps_over_threshold": over,
        "longest_run_over_threshold": longest,
        "verdict": "PASS" if worst < threshold and longest <= 2 else "FAIL",
    }


def flatness(fac, mask, width, height, box):
    """Over the face mask: the shares of the factor at the flat shaded value
    (about 1), at lit (about 0) and between them. A striped face puts many
    pixels between; edge mixing puts a little."""
    x0, x1, y0, y1 = box
    total = full = mid = 0
    for y in range(y0, y1):
        row = y * width
        for x in range(x0, x1):
            if mask[row + x] < 0.5 and fac[row + x] >= 0.02:
                total += 1
                v = fac[row + x]
                if v > 0.98:
                    full += 1
                elif 0.02 < v < 0.98:
                    mid += 1
    if total == 0:
        return None
    return {"face_pixels": total, "flat_shaded_share": round(full / total, 4),
            "in_between_share": round(mid / total, 4)}


def edge(fac, width, height, x0, x1, y0, y1, max_width=150):
    """The sharpest dark-to-bright sweep along a row, 10%..90% width."""
    best = None
    for y in range(y0, y1, 2):
        row = fac[y * width : (y + 1) * width]
        for x in range(x0, x1 - 4):
            a, b = row[x], row[x + 4]
            rise = b - a
            if rise > 0.6 and (best is None or rise > best[0]):
                best = (rise, y, x)
    if best is None:
        return None
    _, y, x = best
    row = fac[y * width : (y + 1) * width]
    # Walk left from the steepest point to the 10% level, right to the 90%.
    left = x
    while left > 0 and row[left] > 0.1:
        left -= 1
    right = x
    while right < width - 1 and row[right] < 0.9:
        right += 1
    return {
        "row_y": y,
        "steepest_x": x,
        "width_px": right - left,
        "from_x": left,
        "to_x": right,
    }


def main(argv):
    fac, width, height = gray(argv[1])
    mask, mwidth, mheight = gray(argv[2], erode=7)
    if (width, height) != (mwidth, mheight):
        print("shadow_stripes: FAIL the two snapshots have different sizes")
        return 1
    # The central picture, past the picture's border and the interface bars.
    box = (width // 8, width * 7 // 8, height // 8, height * 3 // 4)
    checked = stripes(fac, mask, width, height, box)
    if checked is None:
        # Erosion left no face: the view's faces are thin (fences, cards).
        # Report their per-pixel flatness from a lighter erosion instead; a
        # two-pixel fence cannot show a stripe field anyway, its pixels mix
        # with the background.
        thin, _, _ = gray(argv[2], erode=3)
        flat = flatness(fac, thin, width, height, box)
        print(f"shadow_stripes: PASS no sizeable face in the box "
              f"({json.dumps(flat)}), {Path(argv[1]).name}")
        return 0
    result = {"stripes": checked}
    flat = flatness(fac, mask, width, height, box)
    if flat:
        result["flatness"] = flat
    edge_box = (width // 6, width * 5 // 6, height // 3, height * 5 // 6)
    found = edge(fac, width, height, *edge_box)
    if found:
        result["edge"] = found
    worst = checked["worst_column_step"]
    print(f"shadow_stripes: {checked['verdict']} "
          f"worst wall column step {worst:.4f} (under 0.05 passes), "
          f"steps over threshold {checked['steps_over_threshold']}, "
          f"longest run {checked['longest_run_over_threshold']}, "
          f"{Path(argv[1]).name}")
    if flat:
        print(f"  face flatness: {flat['face_pixels']} face pixels, "
              f"{flat['flat_shaded_share']:.4f} at the flat shaded value, "
              f"{flat['in_between_share']:.4f} between")
    if found:
        print(f"  ground edge: row y={found['row_y']} 10-90% width "
              f"{found['width_px']}px (x {found['from_x']}..{found['to_x']})")
    print(json.dumps(result))
    return 0 if checked["verdict"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
