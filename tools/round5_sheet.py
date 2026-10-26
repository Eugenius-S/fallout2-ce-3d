"""The round-5 evidence sheet: before|after at the corner, then the after
views of h8 and the corrals.

py -3 tools/round5_sheet.py
"""

from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
SHOTS = ROOT / "build" / "082-shots"
OUT = ROOT / "runs" / "082-sun-overhead-crisp" / "round5"


def crop(path_glob, box, scale=3):
    src = sorted(path_glob.parent.glob(path_glob.name))[0]
    with Image.open(src) as im:
        piece = im.convert("RGB").crop(box)
        return piece.resize((piece.width * scale, piece.height * scale), Image.NEAREST)


def first(directory):
    return sorted(Path(directory).glob("*.png"))[0]


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    box = (780, 260, 1180, 460)
    before = crop(first(SHOTS / "r5-buckets" / "building-h7-game"), box)
    after7 = crop(first(SHOTS / "r5-flat" / "building-h7-game"), box)
    after8 = crop(first(SHOTS / "r5-flat" / "building-h8-game"), box)
    corrals = crop(first(SHOTS / "r5-flat" / "corrals-h8-game"), (1050, 350, 1550, 750), 2)
    w = before.width + after7.width
    h = before.height + max(after8.height, corrals.height)
    sheet = Image.new("RGB", (w, h + 3), (24, 24, 24))
    sheet.paste(before, (0, 0))
    sheet.paste(after7, (before.width, 0))
    sheet.paste(after8, (0, before.height + 3))
    sheet.paste(corrals, (after8.width, before.height + 3))
    draw = ImageDraw.Draw(sheet)
    sheet.save(OUT / "round5-sheet.jpg", quality=88)
    print(OUT / "round5-sheet.jpg")
    return 0


if __name__ == "__main__":
    import sys
    sys.exit(main())
