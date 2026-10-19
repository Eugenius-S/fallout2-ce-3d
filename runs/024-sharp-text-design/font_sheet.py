"""Font sample sheets: the game's FONT1.AAF at x2 next to TTF candidates."""
import struct
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

repo, fonts_dir, out_dir = (Path(a) for a in sys.argv[1:4])
sys.path.insert(0, str(repo / "tools" / "scene_export" / "vendor"))
from f2lib.dat2 import Dat2  # noqa: E402

SCALE = 2
WIDTH = 640 * SCALE

with Dat2(repo / "game" / "master.dat") as dat:
    aaf_en = dat.read("font1.aaf")
    pal = dat.read(next(n for n in dat.names() if n.lower().endswith("color.pal")))
with Dat2(repo / "game" / "mods" / "rpu_russian.dat") as dat:
    aaf_ru = dat.read("font1.aaf")

palette = [tuple(min(255, c * 4) for c in pal[i * 3:i * 3 + 3]) for i in range(256)]
green = min(palette, key=lambda c: (c[0]) ** 2 + (c[1] - 252) ** 2 + (c[2]) ** 2)
print("green", green)

def load_aaf(aaf):
    global max_h, letter_sp, word_sp, line_sp, glyphs, data
    max_h, letter_sp, word_sp, line_sp = struct.unpack(">4H", aaf[4:12])
    glyphs = [struct.unpack(">HHI", aaf[12 + i * 8:20 + i * 8]) for i in range(256)]
    data = aaf[2060:]
    print("aaf maxHeight", max_h, "letter", letter_sp, "word", word_sp, "line", line_sp)


load_aaf(aaf_en)


def draw_aaf(img, x, y, text, color):
    for byte in text.encode("cp1251"):
        w, h, off = glyphs[byte]
        if byte == 0x20:
            x += word_sp + letter_sp
            continue
        top = y + (max_h - h)
        for gy in range(h):
            for gx in range(w):
                level = data[off + gy * w + gx]
                if level:
                    a = min(level, 7) / 7
                    bg = img.getpixel((x + gx, top + gy))
                    img.putpixel((x + gx, top + gy), tuple(round(bg[i] + (color[i] - bg[i]) * a) for i in range(3)))
        x += w + letter_sp


LINES = {
    "ru": [
        "СТАТУС      12:47   25 ИЮЛЯ 2241",
        "Здоровье: 32/45     Радиация: 12     Яд: 0",
        "Перед вами крепкий мужчина в потёртой кожаной куртке.",
        "1. Привет. Что здесь происходит?",
        "2. Мне нужна работа. Есть что-нибудь на примете?",
    ],
    "en": [
        "STATUS      12:47   25 JULY 2241",
        "Hit Points: 32/45     Radiation: 12     Poison: 0",
        "You see a sturdy man in a worn leather jacket.",
        "1. Hi. What's going on here?",
        "2. I need work. Anything around?",
    ],
}

CANDIDATES = [
    ("PT Sans Bold", "PT_Sans-Web-Bold.ttf", None),
    ("Roboto Medium", "Roboto.ttf", "Medium"),
    ("Fira Sans Medium", "FiraSans-Medium.ttf", None),
    ("Rubik Medium", "Rubik.ttf", "Medium"),
    ("Sofia Sans SemiBold", "SofiaSans.ttf", "SemiBold"),
    ("Exo 2 SemiBold", "Exo2.ttf", "SemiBold"),
    ("IBM Plex Mono SemiBold", "IBMPlexMono-SemiBold.ttf", None),
    ("JetBrains Mono Bold", "JetBrainsMono.ttf", "Bold"),
]

cap_px = glyphs[ord("H")][1] * SCALE
line_px = (max_h + line_sp) * SCALE
BG = (10, 18, 10)
LABEL = (150, 150, 150)
label_font = ImageFont.truetype(str(fonts_dir / "RobotoCondensed.ttf"), 15)


EXTREMES = "HДЩрфуgjy|"


def load(path, style, size):
    font = ImageFont.truetype(str(path), size)
    if style:
        raw = font.get_variation_names()
        names = [n.decode() if isinstance(n, bytes) else n for n in raw]
        font.set_variation_by_name(raw[names.index(style)])
    return font


def matched(path, style, lines, target_width):
    """Width of the game's lines, but never taller than the game's line."""
    size = 8
    while True:
        font = load(path, style, size + 1)
        box = font.getbbox(EXTREMES, anchor="ls")
        wide = max(font.getlength(l) for l in lines)
        if box[3] - box[1] > line_px or wide > target_width:
            font = load(path, style, size)
            fit = "height" if box[3] - box[1] > line_px else "width"
            return font, -font.getbbox(EXTREMES, anchor="ls")[1], fit
        size += 1


def has_cyrillic(font):
    def ink(ch):
        im = Image.new("L", (64, 64))
        ImageDraw.Draw(im).text((4, 4), ch, font=font, fill=255)
        return im.tobytes()

    missing = ink("")
    return all(ink(ch) != missing for ch in "ЖЩЯёЁ")


for lang, lines in LINES.items():
    load_aaf(aaf_ru if lang == "ru" else aaf_en)
    line_px = (max_h + line_sp) * SCALE
    block = 26 + len(lines) * line_px + 14
    img = Image.new("RGB", (WIDTH, block * (1 + len(CANDIDATES))), BG)
    draw = ImageDraw.Draw(img)

    small = Image.new("RGB", (WIDTH // SCALE, len(lines) * (max_h + line_sp)), BG)
    for i, line in enumerate(lines):
        draw_aaf(small, 10, i * (max_h + line_sp), line, green)
    draw.text((20, 6), "game font FONT1.AAF, x2 (now)", font=label_font, fill=LABEL)
    img.paste(small.resize((WIDTH, small.height * SCALE), Image.NEAREST), (0, 26))

    target = max(sum((glyphs[b][0] if b != 0x20 else word_sp) + letter_sp for b in l.encode("cp1251")) for l in lines) * SCALE
    for n, (title, file, style) in enumerate(CANDIDATES, start=1):
        font, rise, fit = matched(fonts_dir / file, style, lines, target)
        top = n * block
        note = "" if has_cyrillic(font) else "  (NO CYRILLIC)"
        draw.text((20, top + 6), f"{title}, {font.size}px, limited by {fit}{note}", font=label_font, fill=LABEL)
        for i, line in enumerate(lines):
            draw.text((20, top + 26 + i * line_px + rise), line, font=font, fill=green, anchor="ls")

    out_dir.mkdir(parents=True, exist_ok=True)
    img.save(out_dir / f"fonts-{lang}.png")
    print("saved", out_dir / f"fonts-{lang}.png", img.size)
