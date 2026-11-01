"""Compose the 125 acceptance pairs: 124's state | this task's, per site and turn."""
from PIL import Image
from pathlib import Path

SRC = Path("runs/_125_shots")
OUT = Path("runs/125-widen-back-views")
OUT.mkdir(parents=True, exist_ok=True)

def capture(directory):
    pngs = [p for p in directory.glob("*.png") if p.name != "reference.png"]
    return max(pngs, key=lambda p: p.stat().st_mtime)

for site_dir in sorted((SRC / "before").iterdir()):
    if not site_dir.is_dir():
        continue
    before = capture(site_dir)
    after = capture(SRC / "after" / site_dir.name)
    a = Image.open(before).convert("RGB")
    b = Image.open(after).convert("RGB")
    w, h = a.size
    pair = Image.new("RGB", (w * 2 + 8, h), (255, 0, 255))
    pair.paste(a, (0, 0))
    pair.paste(b, (w + 8, 0))
    pair = pair.resize((pair.width // 2, pair.height // 2), Image.LANCZOS)
    pair.save(OUT / f"{site_dir.name}-pair.png")
    print(site_dir.name, "pair saved")
