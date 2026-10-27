"""Build task 088 sheets from the isolated native-frame snapshot batches."""
import json
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
SHOTS = ROOT / "build/muzzle088"
OUT = ROOT / "runs/088-muzzle-flash"
GROUPS = {
    "bullets": ("bullets", ("pistol", "rifle", "smg", "minigun")),
    "energy-pistols": ("energy", ("laser-pistol", "plasma-pistol", "pulse-pistol", "dual-plasma")),
    "energy-rifles": ("energy", ("laser-rifle", "plasma-rifle", "pulse-rifle", "gatling-laser")),
    "energy-heavy": ("energy", ("flamethrower", "rocket", "robot-rocket", "boss-plasma")),
}
CELL_W, CELL_H, LABEL = 130, 182, 23


def load_batch(folder):
    entries = [json.loads(path.read_text(encoding="utf-8")) for path in sorted(folder.glob("*.json"))]
    if not entries:
        raise ValueError(f"No captures in {folder}")
    frames = entries[0]["muzzle"]["frames"]
    modes = len(entries) // frames
    if modes not in (2, 3) or len(entries) != modes * frames:
        raise ValueError(f"Incomplete capture in {folder}")
    for i, item in enumerate(entries):
        m = item["muzzle"]
        if m["frames"] != frames or m["frame"] != i % frames or item["exe_sha256"] != entries[0]["exe_sha256"]:
            raise ValueError(f"Frame ordering mismatch in {folder}")
    for frame in range(frames):
        native = entries[frame]["muzzle"]
        control = entries[frames + frame]["muzzle"]
        if control["local_light"] or (modes == 2 and native["local_light"] != bool(native["core_pixels"])):
            raise ValueError(f"Unexpected muzzle light in {folder}, frame {frame}")
        if modes == 2 and not native["core_pixels"]:
            first = Image.open(folder / entries[frame]["image"]).convert("RGB")
            second = Image.open(folder / entries[frames + frame]["image"]).convert("RGB")
            if ImageChops.difference(first, second).getbbox():
                raise ValueError(f"Quiet frame changed in {folder}, frame {frame}")
        if modes == 3:
            extra = entries[2 * frames + frame]["muzzle"]
            if extra["local_light"] != bool(extra["core_pixels"]):
                raise ValueError(f"Unexpected extra light in {folder}, frame {frame}")
            if native["projectile_visible"] != control["projectile_visible"] or native["projectile_visible"] != extra["projectile_visible"]:
                raise ValueError(f"Projectile comparison mismatch in {folder}, frame {frame}")
    return entries, frames, modes


def sheet(name, group, cases):
    batches = [(case, rot, *load_batch(SHOTS / group / f"{case}-r{rot}"))
               for case in cases for rot in (1, 4)]
    width = max(frames for _, _, _, frames, _ in batches) * CELL_W
    height = sum(LABEL + modes * CELL_H for _, _, _, _, modes in batches)
    image = Image.new("RGB", (width, height), (17, 17, 17))
    draw = ImageDraw.Draw(image)
    y = 0
    for case, rot, entries, frames, modes in batches:
        draw.text((5, y + 4), f"{case} | rotation {rot} | frames 0-{frames-1}", fill="white")
        y += LABEL
        labels = ("native + flash", "native only") if modes == 2 else ("projectile / native glow", "without muzzle", "+ small flash")
        folder = SHOTS / group / f"{case}-r{rot}"
        for mode in range(modes):
            for frame in range(frames):
                source = Image.open(folder / entries[mode * frames + frame]["image"]).convert("RGB")
                crop = source.crop((760, 260, 1160, 820)).resize((CELL_W, CELL_H - 20), Image.Resampling.LANCZOS)
                x = frame * CELL_W
                image.paste(crop, (x, y + 20))
                draw.text((x + 3, y + 3), f"{frame}" + (f" | {labels[mode]}" if frame == 0 else ""), fill="white")
            y += CELL_H
    path = OUT / f"{name}.jpg"
    OUT.mkdir(parents=True, exist_ok=True)
    for quality in (88, 80, 72, 64, 56):
        image.save(path, quality=quality, optimize=True)
        if path.stat().st_size <= 1_000_000:
            break
    else:
        raise ValueError(f"Sheet exceeds 1 MB: {path}")
    print(path, image.size, path.stat().st_size)


if __name__ == "__main__":
    for title, (source, names) in GROUPS.items():
        sheet(title, source, names)
