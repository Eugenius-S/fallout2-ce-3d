#!/usr/bin/env python3
"""Check that a PNG pixel has the expected RGB value."""

import sys

from PIL import Image


def main() -> int:
    if len(sys.argv) != 7:
        print("usage: check_png_pixel.py IMAGE X Y RED GREEN BLUE", file=sys.stderr)
        return 2

    path = sys.argv[1]
    x, y, red, green, blue = map(int, sys.argv[2:])
    with Image.open(path) as source:
        image = source.convert("RGB")
        if x < 0 or y < 0 or x >= image.width or y >= image.height:
            print(f"pixel ({x},{y}) is outside {image.width}x{image.height} image", file=sys.stderr)
            return 1
        actual = image.getpixel((x, y))

    expected = (red, green, blue)
    if actual != expected:
        print(f"pixel ({x},{y}) is {actual}, expected {expected}", file=sys.stderr)
        return 1
    print(f"pixel ({x},{y}) matches RGB {expected}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
