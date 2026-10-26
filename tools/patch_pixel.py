"""Print the average color of a small patch of an image.

py -3 tools/patch_pixel.py <image> <x> <y> [radius]
"""

import sys
from pathlib import Path

from PIL import Image


def main(argv):
    src = Path(argv[1])
    x, y = int(argv[2]), int(argv[3])
    r = int(argv[4]) if len(argv) > 4 else 2
    with Image.open(src) as im:
        im = im.convert("RGB")
        box = im.crop((x - r, y - r, x + r + 1, y + r + 1))
        pixels = list(box.getdata())
        n = len(pixels)
        avg = tuple(sum(c[i] for c in pixels) // n for i in range(3))
    print(f"{src.name} @({x},{y}) r{r}: rgb{avg}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
