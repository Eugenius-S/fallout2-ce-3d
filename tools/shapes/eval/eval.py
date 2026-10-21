"""The shapes eval (docs/2026-10-20-shapes-and-rotation-design.md).

    py -3 tools/shapes/eval/eval.py shoot [--only a.frm,b.frm]   # turned views, test instance
    py -3 tools/shapes/eval/eval.py sheets                       # contact sheets
    py -3 tools/shapes/eval/eval.py score [--scenes DIR]         # the score
    py -3 tools/shapes/eval/eval.py check [--threshold T]        # things that vanish from the side

Shots and sheets go to build/shapes-eval/. Each FRM's instance is seen alone
(dev-snapshot -Solo) from 0, 45, 90, 180 and 270 degrees round it at the game
camera's pitch, in full daylight. An FRM passes when the engine gives it its labelled shape and
its by-eye verdict (verdicts.json, written after looking at its sheet) is
"pass". labels.json is frozen: no change without the owner.

sheets keeps the previous run's sheets in build/shapes-eval/previous/, writes
build/shapes-eval/changed.png with only the sheets that changed (each as
previous over new) and marks those FRMs' verdicts "stale" until someone
judges them again; score counts a stale verdict as not judged.

check shoots every view twice (solo radius 1, then radius 0, which keeps
nothing but the tile itself) into shots/ and empties/, counts the pixels of
the thing seen from each angle and flags an FRM whose side views fall under
the threshold share of its game view. Flats baked into the floor measure
nothing and are skipped.
"""
from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
OUT = ROOT / "build" / "shapes-eval"
ANGLES = (0, 45, 90, 180, 270)
sys.path.insert(0, str(HERE.parent))

from inventory import todays_shapes  # noqa: E402

sys.path.insert(0, str(ROOT / "tools" / "scene_export"))
from baked_sidecar import FLAT, FLY_FLAT, PLANE, decode_baked_sidecar  # noqa: E402

# How scene.json's shapes of today read in the labels' words.
TODAY = {"card": "card", "card (donor: crossed)": "card", "card (door)": "card", "flat": "flat",
         "panel": "plane", "box": "box", "box+back": "box+back", "drum": "drum", "part of a solid": "boxes"}


def labels():
    return json.loads((HERE / "labels.json").read_text(encoding="utf-8"))


def verdicts():
    path = HERE / "verdicts.json"
    return json.loads(path.read_text(encoding="utf-8")) if path.is_file() else {}


def save_verdicts(data):
    (HERE / "verdicts.json").write_text(json.dumps(data, indent=1, ensure_ascii=False) + "\n",
                                        encoding="utf-8")


# The test instance is deterministic, so a real change is many pixels; this
# much slack keeps trivial jitter from re-marking a sheet changed.
DIFF_CHANNEL = 8
DIFF_PIXELS = 32


def sheets_differ(previous, new):
    """Whether two sheet images differ beyond the tolerance."""
    from PIL import ImageChops
    if previous.size != new.size:
        return True
    red, green, blue = ImageChops.difference(previous, new).split()
    strongest = ImageChops.lighter(ImageChops.lighter(red, green), blue)
    return sum(strongest.histogram()[DIFF_CHANNEL:]) > DIFF_PIXELS


def mark_stale(frms):
    """A changed sheet invalidates its FRM's verdict until it is judged again."""
    data = verdicts()
    marked = []
    for frm in frms:
        entry = data.get(frm)
        if isinstance(entry, dict) and entry.get("verdict"):
            entry["stale"] = True
            marked.append(frm)
    if marked:
        save_verdicts(data)
    return marked


def shoot(only):
    """All views of one map elevation in one run of the test instance: a map
    tour (dev-snapshot -Tour) whose stops each show one thing alone from one
    angle, each followed by its empty twin (solo radius 0). Its first
    snapshot is the start, before the tour."""
    groups = {}
    for label in labels():
        if not only or label["frm"] in only:
            groups.setdefault(tuple(label["instance"][:2]), []).append(label)
    for (name, elevation), group in groups.items():
        stops, order = [], []
        for label in group:
            distance = 6 if label["big"] else 3.5
            for angle in ANGLES:
                tile = label["instance"][2]
                stops.append(f"{name}:{elevation}:{tile}:1:{angle}:{distance}")
                order.append((label["frm"], angle, False))
                stops.append(f"{name}:{elevation}:{tile}:0:{angle}:{distance}")
                order.append((label["frm"], angle, True))
        folder = OUT / "batch" / f"{name}-{elevation}"
        if folder.exists():
            shutil.rmtree(folder)
        folder.mkdir(parents=True)
        # -Command, quoted: through -File a list with commas becomes an array.
        command = (f"& '{ROOT / 'tools' / 'dev-snapshot.ps1'}' -Map {name} -Elevation {elevation} "
                   f"-Tour '{','.join(stops)}' -Ambient 65536 -OutputDirectory '{folder}' "
                   f"-TimeoutSeconds {60 + 3 * len(stops)}")
        run = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", command],
                             capture_output=True, text=True, errors="replace")
        if run.returncode != 0:
            raise SystemExit(f"{name} e{elevation}: dev-snapshot failed: {run.stderr[-600:]}")
        shots = sorted(folder.glob("*.png"))[1:]
        if len(shots) != len(order):
            raise SystemExit(f"{name} e{elevation}: {len(shots)} views for {len(order)} stops")
        for (frm, angle, empty), shot in zip(order, shots):
            tree = OUT / ("empties" if empty else "shots") / frm / str(angle)
            if tree.exists():
                shutil.rmtree(tree)
            tree.mkdir(parents=True)
            shutil.copy(shot, tree / shot.name)
        print(name, elevation, len(shots) // 2, "views", flush=True)


def sheets():
    from PIL import Image, ImageDraw
    cell_w, cell_h, head = 300, 240, 18
    pages = []
    changed = []
    fresh = []
    for label in labels():
        row = Image.new("RGB", (cell_w * len(ANGLES), cell_h + head), (24, 24, 24))
        draw = ImageDraw.Draw(row)
        verdict = verdicts().get(label["frm"], {})
        draw.text((4, 2), f"{label['frm']}  {label['name']}  label: {label['shape']}  {label['note']}  "
                  f"verdict: {verdict.get('verdict', '-')} {verdict.get('why', '')}", fill=(255, 230, 0))
        for column, angle in enumerate(ANGLES):
            shots = sorted((OUT / "shots" / label["frm"] / str(angle)).glob("*.png"))
            if not shots:
                continue
            image = Image.open(shots[-1]).convert("RGB")
            w, h = image.size
            image = image.crop((int(w * 0.28), int(h * 0.12), int(w * 0.72), int(h * 0.82)))
            image.thumbnail((cell_w - 4, cell_h - 4))
            row.paste(image, (column * cell_w + 2, head + 2))
            draw.text((column * cell_w + 4, head + 4), f"{angle}", fill=(255, 255, 255))
        (OUT / "sheets").mkdir(parents=True, exist_ok=True)
        row.save(OUT / "sheets" / f"{label['frm']}.png")
        previous = OUT / "previous" / f"{label['frm']}.png"
        if previous.is_file():
            if sheets_differ(Image.open(previous).convert("RGB"), row):
                changed.append(label["frm"])
        else:
            fresh.append(label["frm"])
        pages.append(row)
    for start in range(0, len(pages), 8):
        part = pages[start:start + 8]
        page = Image.new("RGB", (part[0].width, sum(p.height for p in part)))
        y = 0
        for p in part:
            page.paste(p, (0, y))
            y += p.height
        page.save(OUT / f"page{start // 8 + 1}.png")
    # The next run compares against these sheets.
    (OUT / "previous").mkdir(parents=True, exist_ok=True)
    for label in labels():
        shutil.copyfile(OUT / "sheets" / f"{label['frm']}.png", OUT / "previous" / f"{label['frm']}.png")
    if changed:
        gap, divider = 2, 4
        height = len(changed) * (2 * pages[0].height + gap + divider) - divider
        sheet = Image.new("RGB", (pages[0].width, height), (120, 30, 30))
        y = 0
        for frm in changed:
            sheet.paste(Image.open(OUT / "previous" / f"{frm}.png").convert("RGB"), (0, y))
            y += pages[0].height + gap
            sheet.paste(Image.open(OUT / "sheets" / f"{frm}.png").convert("RGB"), (0, y))
            y += pages[0].height + gap + divider
        sheet.save(OUT / "changed.png")
        stale = mark_stale(changed)
        print(f"changed sheets: {', '.join(changed)}")
        if stale:
            print(f"verdicts marked stale until judged again: {', '.join(stale)}")
    else:
        changed_png = OUT / "changed.png"
        if changed_png.exists():
            changed_png.unlink()
        note = f"; first baseline for {len(fresh)} FRMs" if fresh else ""
        print(f"sheets unchanged since the previous run{note}")
    print(OUT / "sheets")


def score(scenes):
    judged = verdicts()
    cache = {}
    baked = {}
    passed = weighted = total = stale = 0
    for label in labels():
        entry = judged.get(label["frm"], {})
        if entry.get("stale"):
            stale += 1
        name, elevation, tile, serial, fid = label["instance"]
        key = (name, elevation)
        if key not in cache:
            cache[key] = todays_shapes(json.loads((scenes / name / str(elevation) / "scene.json")
                                                  .read_text(encoding="utf-8")))
        shape = TODAY.get(cache[key].get(serial, "not drawn"), "none")
        if shape == "card":  # the engine may lay a card flat or on a plane (baked.bin)
            modes = baked.setdefault(key, {(record[0], record[1]): record[2] for record in decode_baked_sidecar(
                (scenes / name / str(elevation) / "baked.bin").read_bytes())})
            shape = {FLAT: "flat", FLY_FLAT: "flat", PLANE: "plane"}.get(modes.get((tile, fid)), shape)
        # A box with a back is still a box; a plain box is no box with a back.
        kind = shape == label["shape"] or (label["shape"], shape) == ("box", "box+back")
        eye = entry.get("verdict") == "pass" and not entry.get("stale")
        ok = kind and eye
        passed += ok
        weighted += label["count"] * ok
        total += label["count"]
        print(f"{'PASS' if ok else 'fail'}  {label['frm']:13} label {label['shape']:8} engine {shape:7} "
              f"eye {'stale' if entry.get('stale') else entry.get('verdict', '-')}")
    stale_note = f"; {stale} stale, not judged" if stale else ""
    print(f"score {passed}/{len(labels())} = {100 * passed / len(labels()):.0f}%, "
          f"weighted by how often they stand {100 * weighted / total:.0f}%{stale_note}")


# A pixel belongs to the thing when its strongest channel differs by this
# much; with fewer thing pixels than this from the game view the FRM is not
# measurable (a flat baked into the floor survives the empty solo).
THING_CHANNEL = 8
THING_PIXELS = 200


def thing_pixels(thing_path, empty_path):
    """The pixels of the thing: what its solo shot shows over the empty one."""
    from PIL import Image, ImageChops
    thing = Image.open(thing_path).convert("RGB")
    empty = Image.open(empty_path).convert("RGB")
    if thing.size != empty.size:
        raise SystemExit(f"{thing_path} and {empty_path} differ in size")
    red, green, blue = ImageChops.difference(thing, empty).split()
    strongest = ImageChops.lighter(ImageChops.lighter(red, green), blue)
    return sum(strongest.histogram()[THING_CHANNEL:])


def check(threshold):
    """How much of the thing each turned view still shows against the game's
    own view (angle 0): the solo shots counted over their empty twins. Flags
    a thing that shrinks to a sliver or disappears from a side."""
    print(f"threshold {threshold}: flagged when some side view shows under "
          f"{threshold:.0%} of what the game view shows")
    flagged = []
    measurable = 0
    for label in labels():
        counts = []
        for angle in ANGLES:
            shots = sorted((OUT / "shots" / label["frm"] / str(angle)).glob("*.png"))
            empties = sorted((OUT / "empties" / label["frm"] / str(angle)).glob("*.png"))
            if not shots or not empties:
                counts = None
                break
            counts.append(thing_pixels(shots[-1], empties[-1]))
        if counts is None:
            print(f"????  {label['frm']:13} no shots, run shoot")
            continue
        base = counts[0]
        if base < THING_PIXELS:
            print(f"skip  {label['frm']:13} flat in the floor, {base} px from the game view")
            continue
        measurable += 1
        shares = " ".join(f"{count / base:.2f}" for count in counts[1:])
        worst = min(counts[1:]) / base
        hit = worst < threshold
        flagged.append((label["frm"], hit))
        eye = verdicts().get(label["frm"], {}).get("verdict", "-")
        print(f"{'FLAG' if hit else 'ok  '} {label['frm']:13} game view {base:6} px, "
              f"45/90/180/270: {shares}   eye {eye}")
    hits = [frm for frm, hit in flagged if hit]
    print(f"{len(hits)} flagged of {measurable} measurable"
          + (f": {', '.join(hits)}" if hits else ""))
    judged = verdicts()
    seen_wrong = [frm for frm, hit in flagged if hit and judged.get(frm, {}).get("verdict") == "pass"]
    missed = [frm for frm, hit in flagged if not hit and judged.get(frm, {}).get("verdict") == "fail"]
    if seen_wrong:
        print(f"flagged but the eye passed them: {', '.join(seen_wrong)}")
    if missed:
        print(f"not flagged but the eye failed them: {', '.join(missed)}")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("command", choices=("shoot", "sheets", "score", "check"))
    parser.add_argument("--only", help="comma-separated FRMs (shoot)")
    parser.add_argument("--threshold", type=float, default=0.3,
                        help="the share under which check flags an FRM (default 0.3)")
    # A worktree's own export (AGENTS.md), else the game's mod cache.
    parser.add_argument("--scenes", type=Path, default=next(
        path for path in (ROOT / "build" / "scene-cache", ROOT / "game" / "mod-cache" / "scene",
                          ROOT.parent / "fallout2-ce-3d" / "game" / "mod-cache" / "scene") if path.is_dir()))
    args = parser.parse_args()
    if args.command == "shoot":
        shoot(set(args.only.split(",")) if args.only else None)
    elif args.command == "sheets":
        sheets()
    elif args.command == "check":
        check(args.threshold)
    else:
        score(args.scenes)


if __name__ == "__main__":
    main()
