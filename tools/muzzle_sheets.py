"""Build task 088 sheets from the isolated native-frame snapshot batches."""
import json
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
SHOTS = ROOT / "build/muzzle088"
OUT = ROOT / "runs/088-muzzle-flash"
GROUPS = {
    "bullets": ("bullets", ("pistol", "rifle", "smg", "minigun")),
    "energy-pistols": ("energy", ("laser-pistol", "plasma-pistol", "pulse-pistol", "dual-plasma")),
    "energy-rifles": ("energy", ("laser-rifle", "plasma-rifle", "pulse-rifle", "gatling-laser")),
    "energy-heavy": ("energy", ("flamethrower", "rocket", "robot-rocket", "boss-plasma")),
}
CELL_W, CELL_H, LABEL = 150, 210, 25


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
    batches = [(case, [load_batch(SHOTS / group / f"{case}-r{rot}") for rot in (1, 4)]) for case in cases]
    width = max(sum(batch[1] for batch in pair) for _, pair in batches) * CELL_W + 12
    def rows(batch):
        entries, _, modes = batch
        return (0, 2) if modes == 3 and entries[0]["muzzle"]["projectile_fid"] > 0 else tuple(range(modes))
    height = sum(LABEL + max(len(rows(batch)) for batch in pair) * CELL_H for _, pair in batches)
    image = Image.new("RGB", (width, height), (17, 17, 17))
    draw = ImageDraw.Draw(image)
    try:
        font = ImageFont.truetype("arial.ttf", 14)
    except OSError:
        font = ImageFont.load_default()
    y = 0
    for case, pair in batches:
        x_start = 0
        for rot, (entries, frames, modes) in zip((1, 4), pair):
            draw.text((x_start + 5, y + 4), f"{case} | rotation {rot} | frames 0-{frames-1}", fill="white", font=font)
            labels = ("native + flash", "native only") if modes == 2 else ("native glow", "no muzzle", "+ small flash")
            folder = SHOTS / group / f"{case}-r{rot}"
            for row, mode in enumerate(rows((entries, frames, modes))):
                row_y = y + LABEL + row * CELL_H
                for frame in range(frames):
                    source = Image.open(folder / entries[mode * frames + frame]["image"]).convert("RGB")
                    crop = source.crop((760, 260, 1160, 820)).resize((CELL_W, CELL_H - 20), Image.Resampling.LANCZOS)
                    x = x_start + frame * CELL_W
                    image.paste(crop, (x, row_y + 20))
                    draw.text((x + 3, row_y + 2), f"{frame}" + (f" | {labels[mode]}" if frame == 0 else ""), fill="white", font=font)
            x_start += frames * CELL_W + 12
        y += LABEL + max(len(rows(batch)) for batch in pair) * CELL_H
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
