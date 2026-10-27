"""Crop and optionally scale regions of a shot for the eye review.

py -3 tools/crop_box.py <image> <left> <top> <right> <bottom> <out> [scale]
"""

import sys
from pathlib import Path

from PIL import Image


def main(argv):
    src = Path(argv[1])
    left, top, right, bottom = (int(v) for v in argv[2:6])
    out = Path(argv[6])
    scale = int(argv[7]) if len(argv) > 7 else 2
    with Image.open(src) as im:
        box = im.convert("RGB").crop((left, top, right, bottom))
        if scale > 1:
            box = box.resize((box.width * scale, box.height * scale), Image.NEAREST)
        out.parent.mkdir(parents=True, exist_ok=True)
        box.save(out, quality=92)
    print(out)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
