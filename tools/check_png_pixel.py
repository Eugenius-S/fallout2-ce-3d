"""Assert that one decoded PNG pixel has an expected RGB color."""

import argparse
from pathlib import Path

from PIL import Image


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    parser.add_argument("x", type=int)
    parser.add_argument("y", type=int)
    parser.add_argument("red", type=int)
    parser.add_argument("green", type=int)
    parser.add_argument("blue", type=int)
    args = parser.parse_args()

    with Image.open(args.image) as source:
        image = source.convert("RGB")
        if not (0 <= args.x < image.width and 0 <= args.y < image.height):
            parser.error(f"pixel ({args.x}, {args.y}) is outside {image.width}x{image.height}")
        expected = (args.red, args.green, args.blue)
        actual = image.getpixel((args.x, args.y))
    print(f"CURSOR_PNG_RGB physical={args.x},{args.y} expected={expected} actual={actual}")
    if actual != expected:
        raise SystemExit("cursor PNG pixel does not match engine cursor palette color")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
