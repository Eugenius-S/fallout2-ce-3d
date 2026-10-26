"""The fly tour (task 078): fixed turned views of every Klamath map, before | after.

    py -3 tools/tour/tour.py --before SIDE [--after SIDE] [--maps kladwtwn,klamall] [--reuse] [--out DIR]

A side is either a checkout (its build/fallout2-ce.exe and the scenes its own
tools/dev-snapshot.ps1 reads) or a scene cache folder (<map>/<elevation>/
scene.json, from export.py --cache-root), shot with this checkout's build.
--after defaults to this checkout. So a scene change compares main's cache
(or a checkout of main) against the branch, and an engine change compares two
checkouts.

Every spot of spots.json (this folder; the one spot list) is seen from the
fly camera at each of its turns (077: within 45 degrees), at the game
camera's pitch, in full daylight. One game run per map elevation and side: a
map tour (dev-snapshot -Tour), as the shapes eval shoots.

Shots go to build/tour/<before|after>/<map>-e<elevation>/, sheets to
build/tour/sheets/ (or --out): one JPEG per map elevation (pages of
ROWS_PER_PAGE spots), each row a spot and each turn a before | after pair,
and changed.jpg with only the spots whose views differ, a red frame round
each changed pair. --reuse keeps a side's shots when the same side and stop
list were shot before. Judging the views stays with the eye.
"""
from __future__ import annotations

import argparse
import io
import json
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
OUT = ROOT / "build" / "tour"
ROWS_PER_PAGE = 8
# Each cell shows the middle of the shot: the spot sits at the screen centre.
CROP = (0.2, 0.15, 0.8, 0.85)
CELL_W, CELL_H, HEAD, PAIR_GAP, TURN_GAP = 330, 214, 20, 2, 12
# Runs in the test instance are deterministic, so a real change is many
# pixels (the shapes eval's tolerance).
DIFF_CHANNEL = 8
DIFF_PIXELS = 32
MAX_JPEG = 1_000_000  # CI refuses larger files in runs/
# A view this black in its middle shows nothing: the spot is off the map.
EMPTY_SHARE = 0.9


def load_spots(maps):
    data = json.loads((HERE / "spots.json").read_text(encoding="utf-8"))
    groups = {}
    for spot in data["spots"]:
        if not maps or spot["map"] in maps:
            groups.setdefault((spot["map"], spot["elevation"]), []).append(spot)
    if not groups:
        raise SystemExit("no spots for those maps")
    return data["turns"], data.get("distance", -1), groups


def side_kind(side):
    """('checkout', root) or ('cache', folder)."""
    path = Path(side).resolve()
    if (path / "build" / "fallout2-ce.exe").is_file() and (path / "tools" / "dev-snapshot.ps1").is_file():
        return "checkout", path
    if any(path.glob("*/*/scene.json")):
        return "cache", path
    raise SystemExit(f"{side}: neither a built checkout nor a scene cache")


def shoot(name, side, turns, distance, groups, reuse):
    kind, path = side_kind(side)
    root = path if kind == "checkout" else ROOT
    if kind == "checkout" and not (path / "build" / "scene-cache").is_dir():
        print(f"{name}: {path} has no build/scene-cache, so its scenes come from the game's mod-cache", flush=True)
    for (map_name, elevation), spots in groups.items():
        stops = [f"{map_name}:{elevation}:{spot['tile']}:-1:{turn}:{spot.get('distance', distance)}"
                 for spot in spots for turn in turns]
        folder = OUT / name / f"{map_name}-e{elevation}"
        stamp = {"side": str(path), "kind": kind, "stops": stops}
        stamp_file = folder / "tour.json"
        if (reuse and stamp_file.is_file() and json.loads(stamp_file.read_text(encoding="utf-8")) == stamp
                and len(list(folder.glob("*.png"))) == len(stops) + 1):
            print(f"{name} {map_name} e{elevation}: reused", flush=True)
            continue
        if folder.exists():
            shutil.rmtree(folder)
        folder.mkdir(parents=True)
        # -Command, quoted: through -File a list with commas becomes an array.
        command = (f"& '{root / 'tools' / 'dev-snapshot.ps1'}' -Map {map_name} -Elevation {elevation} "
                   f"-Tour '{','.join(stops)}' -Still -Ambient 65536 -OutputDirectory '{folder}' "
                   f"-TimeoutSeconds {90 + 3 * len(stops)}")
        if kind == "cache":
            command += f" -SceneCache '{path}'"
        run = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", command],
                             capture_output=True, text=True, errors="replace")
        if run.returncode != 0:
            raise SystemExit(f"{name} {map_name} e{elevation}: dev-snapshot failed: {run.stderr[-600:]}")
        # The first snapshot is the start, before the tour.
        shots = sorted(folder.glob("*.png"))
        if len(shots) != len(stops) + 1:
            raise SystemExit(f"{name} {map_name} e{elevation}: {len(shots) - 1} views for {len(stops)} stops")
        stamp_file.write_text(json.dumps(stamp, indent=1) + "\n", encoding="utf-8")
        print(f"{name} {map_name} e{elevation}: {len(stops)} views", flush=True)


def views(name, map_name, elevation):
    return sorted((OUT / name / f"{map_name}-e{elevation}").glob("*.png"))[1:]


def changed_pixels(before, after):
    from PIL import ImageChops
    if before.size != after.size:
        return before.width * before.height
    red, green, blue = ImageChops.difference(before, after).split()
    strongest = ImageChops.lighter(ImageChops.lighter(red, green), blue)
    return sum(strongest.histogram()[DIFF_CHANNEL:])


def empty(image):
    middle = cell(image).convert("L")
    return sum(middle.histogram()[:8]) > EMPTY_SHARE * middle.width * middle.height


def font(size):
    from PIL import ImageFont
    try:
        return ImageFont.load_default(size=size)
    except TypeError:
        return ImageFont.load_default()


def cell(image):
    w, h = image.size
    image = image.crop((int(w * CROP[0]), int(h * CROP[1]), int(w * CROP[2]), int(h * CROP[3])))
    image.thumbnail((CELL_W, CELL_H))
    return image


def spot_row(spot, map_name, elevation, turns, pairs):
    """One spot: a header, then a before | after pair per turn. Returns the
    row image and whether any pair changed."""
    from PIL import Image, ImageDraw
    pair_w = 2 * CELL_W + PAIR_GAP
    row = Image.new("RGB", (len(turns) * pair_w + (len(turns) - 1) * TURN_GAP, HEAD + CELL_H), (24, 24, 24))
    draw = ImageDraw.Draw(row)
    any_changed = False
    notes = []
    for column, (turn, (before, after)) in enumerate(zip(turns, pairs)):
        x = column * (pair_w + TURN_GAP)
        before_image = Image.open(before).convert("RGB")
        after_image = Image.open(after).convert("RGB")
        count = changed_pixels(before_image, after_image)
        changed = count > DIFF_PIXELS
        any_changed |= changed
        if changed:
            notes.append(f"{turn:+d}: {count} px")
        if empty(after_image):
            print(f"empty view: {map_name} e{elevation} tile {spot['tile']} turn {turn:+d}")
        row.paste(cell(before_image), (x, HEAD))
        row.paste(cell(after_image), (x + CELL_W + PAIR_GAP, HEAD))
        label = font(12)
        draw.text((x + 4, HEAD + 3), f"before {turn:+d}", fill=(255, 255, 255), font=label)
        draw.text((x + CELL_W + PAIR_GAP + 4, HEAD + 3), f"after {turn:+d}", fill=(255, 255, 255), font=label)
        if changed:
            draw.rectangle((x, HEAD, x + pair_w - 1, HEAD + CELL_H - 1), outline=(230, 40, 40), width=3)
    status = f"changed ({', '.join(notes)})" if any_changed else "same"
    draw.text((4, 3), f"{map_name} e{elevation}  tile {spot['tile']}  {spot['name']}  {status}",
              fill=(255, 90, 90) if any_changed else (255, 230, 0), font=font(14))
    return row, any_changed


def save_page(rows, path):
    """Rows stacked into one JPEG under the size CI accepts."""
    from PIL import Image
    page = Image.new("RGB", (max(r.width for r in rows), sum(r.height for r in rows) + 4 * (len(rows) - 1)),
                     (60, 60, 60))
    y = 0
    for r in rows:
        page.paste(r, (0, y))
        y += r.height + 4
    for quality in (80, 72, 64, 56):
        buffer = io.BytesIO()
        page.save(buffer, "JPEG", quality=quality, optimize=True)
        if buffer.tell() <= MAX_JPEG:
            break
    path.write_bytes(buffer.getvalue())


def sheets(turns, groups, out):
    if out.exists():
        for old in out.glob("*.jpg"):
            old.unlink()
    out.mkdir(parents=True, exist_ok=True)
    changed_rows = []
    total = changed = 0
    for (map_name, elevation), spots in groups.items():
        before, after = views("before", map_name, elevation), views("after", map_name, elevation)
        expected = len(spots) * len(turns)
        if len(before) != expected or len(after) != expected:
            raise SystemExit(f"{map_name} e{elevation}: {len(before)} before and {len(after)} after views, "
                             f"{expected} expected; shoot first")
        rows = []
        for index, spot in enumerate(spots):
            part = slice(index * len(turns), (index + 1) * len(turns))
            row, differs = spot_row(spot, map_name, elevation, turns, list(zip(before[part], after[part])))
            rows.append(row)
            total += 1
            if differs:
                changed += 1
                changed_rows.append(row)
                print(f"changed: {map_name} e{elevation} tile {spot['tile']} {spot['name']}")
        pages = [rows[start:start + ROWS_PER_PAGE] for start in range(0, len(rows), ROWS_PER_PAGE)]
        for number, page in enumerate(pages, 1):
            suffix = f"-{number}" if len(pages) > 1 else ""
            save_page(page, out / f"{map_name}-e{elevation}{suffix}.jpg")
    for start in range(0, len(changed_rows), ROWS_PER_PAGE):
        suffix = f"-{start // ROWS_PER_PAGE + 1}" if len(changed_rows) > ROWS_PER_PAGE else ""
        save_page(changed_rows[start:start + ROWS_PER_PAGE], out / f"changed{suffix}.jpg")
    print(f"{changed} of {total} spots changed; sheets in {out}")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--before", required=True, help="a built checkout or a scene cache folder")
    parser.add_argument("--after", default=str(ROOT), help="the same (default: this checkout)")
    parser.add_argument("--maps", help="comma-separated map names (default: every map in spots.json)")
    parser.add_argument("--reuse", action="store_true", help="keep shots of the same side and stops")
    parser.add_argument("--out", type=Path, default=OUT / "sheets", help="where the sheets go")
    args = parser.parse_args()
    turns, distance, groups = load_spots(set(args.maps.split(",")) if args.maps else None)
    shoot("before", args.before, turns, distance, groups, args.reuse)
    shoot("after", args.after, turns, distance, groups, args.reuse)
    sheets(turns, groups, args.out)


if __name__ == "__main__":
    sys.exit(main())
