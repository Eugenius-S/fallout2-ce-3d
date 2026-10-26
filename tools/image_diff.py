"""Count differing pixels between two same-size images; save an amplified
diff mask.

py -3 tools/image_diff.py <a> <b> <mask out>
"""

import sys
from pathlib import Path

from PIL import Image, ImageChops


def main(argv):
    a, b, out = (Path(p) for p in argv[1:4])
    with Image.open(a) as ia, Image.open(b) as ib:
        if ia.size != ib.size:
            print(f"sizes differ: {ia.size} vs {ib.size}")
            return 1
        da = ia.convert("RGB")
        db = ib.convert("RGB")
        diff = ImageChops.difference(da, db)
        bbox = diff.getbbox()
        gray = diff.convert("L")
        changed = sum(1 for v in gray.getdata() if v > 8)
        print(f"changed(>8): {changed}  bbox: {bbox}")
        if changed:
            amp = gray.point(lambda v: 255 if v > 8 else 0)
            amp.save(out)
            print(out)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
