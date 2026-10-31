"""Compose the 120 acceptance pairs: 119's reviewed state | this task's, per target and kind."""
from PIL import Image
from pathlib import Path

SRC = Path("runs/_120_shots")
OUT = Path("runs/120-scenery-3d-everywhere")
OUT.mkdir(parents=True, exist_ok=True)

def capture(directory):
    pngs = [p for p in directory.glob("*.png") if p.name != "reference.png"]
    return max(pngs, key=lambda p: p.stat().st_mtime)

names = sorted({d.name.rsplit('-', 2)[0] for d in SRC.iterdir() if d.is_dir()})
for name in names:
    for kind in ("game", "fly"):
        before, after = SRC / f"{name}-{kind}-before", SRC / f"{name}-{kind}-after"
        if not before.is_dir() or not after.is_dir():
            continue
        a = Image.open(capture(before)).convert("RGB")
        b = Image.open(capture(after)).convert("RGB")
        w, h = a.size
        pair = Image.new("RGB", (w * 2 + 8, h), (255, 0, 255))
        pair.paste(a, (0, 0))
        pair.paste(b, (w + 8, 0))
        pair = pair.resize((pair.width // 2, pair.height // 2), Image.LANCZOS)
        pair.save(OUT / f"{name}-{kind}-pair.jpg", quality=88)
        print(name, kind, "pair saved")
