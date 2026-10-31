"""Compose the 120 carve pairs: the game's card (before) | the carved solid
(after), per car and fly quarter."""
from PIL import Image
from pathlib import Path

SRC = Path("runs/_120c_shots")
OUT = Path("runs/120-scenery-3d-everywhere")
OUT.mkdir(parents=True, exist_ok=True)

def capture(directory):
    pngs = [p for p in directory.glob("*.png") if p.name != "reference.png"]
    return max(pngs, key=lambda p: p.stat().st_mtime)

names = sorted({d.name.rsplit('-turn', 1)[0] for d in SRC.iterdir() if d.is_dir()})
for name in names:
    for turn in (0, 90, 180, 270):
        before, after = SRC / f"{name}-turn{turn}-before", SRC / f"{name}-turn{turn}-after"
        if not before.is_dir() or not after.is_dir():
            continue
        a = Image.open(capture(before)).convert("RGB")
        b = Image.open(capture(after)).convert("RGB")
        w, h = a.size
        pair = Image.new("RGB", (w * 2 + 8, h), (255, 0, 255))
        pair.paste(a, (0, 0))
        pair.paste(b, (w + 8, 0))
        pair = pair.resize((pair.width // 2, pair.height // 2), Image.LANCZOS)
        pair.save(OUT / f"{name}-turn{turn}-pair.jpg", quality=88)
        print(name, turn, "pair saved")
