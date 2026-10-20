"""The shapes eval (docs/2026-10-20-shapes-and-rotation-design.md).

    py -3 tools/shapes/eval/eval.py shoot [--only a.frm,b.frm]   # turned views, test instance
    py -3 tools/shapes/eval/eval.py sheets                       # contact sheets
    py -3 tools/shapes/eval/eval.py score [--scenes DIR]         # the score

Shots and sheets go to build/shapes-eval/. Each FRM's instance is seen alone
(dev-snapshot -Solo) from 0, 45, 90, 180 and 270 degrees round it at the game
camera's pitch. An FRM passes when the engine gives it its labelled shape and
its by-eye verdict (verdicts.json, written after looking at its sheet) is
"pass". labels.json is frozen: no change without the owner.
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
OUT = ROOT / "build" / "shapes-eval"
ANGLES = (0, 45, 90, 180, 270)
sys.path.insert(0, str(HERE.parent))

from inventory import todays_shapes  # noqa: E402

# How scene.json's shapes of today read in the labels' words.
TODAY = {"card": "card", "card (donor: crossed)": "card", "card (door)": "card", "flat": "flat",
         "panel": "plane", "box": "box", "drum": "drum", "part of a solid": "boxes"}


def labels():
    return json.loads((HERE / "labels.json").read_text(encoding="utf-8"))


def verdicts():
    path = HERE / "verdicts.json"
    return json.loads(path.read_text(encoding="utf-8")) if path.is_file() else {}


def shoot(only):
    for label in labels():
        if only and label["frm"] not in only:
            continue
        name, elevation, tile, _ = label["instance"]
        for angle in ANGLES:
            folder = OUT / "shots" / label["frm"] / str(angle)
            folder.mkdir(parents=True, exist_ok=True)
            for old in folder.glob("*"):
                old.unlink()
            subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                            str(ROOT / "tools" / "dev-snapshot.ps1"), "-Map", name, "-Elevation", str(elevation),
                            "-ViewTile", str(tile), "-Solo", "1", "-Fly", "-FlyTurn", str(angle or 0.01),
                            "-FlyDistance", "6" if label["big"] else "3.5", "-OutputDirectory", str(folder)],
                           check=True, capture_output=True)
            print(label["frm"], angle, flush=True)


def sheets():
    from PIL import Image, ImageDraw
    cell_w, cell_h, head = 300, 240, 18
    pages = []
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
        pages.append(row)
    for start in range(0, len(pages), 8):
        part = pages[start:start + 8]
        page = Image.new("RGB", (part[0].width, sum(p.height for p in part)))
        y = 0
        for p in part:
            page.paste(p, (0, y))
            y += p.height
        page.save(OUT / f"page{start // 8 + 1}.png")
    print(OUT / "sheets")


def score(scenes):
    judged = verdicts()
    cache = {}
    passed = weighted = total = 0
    for label in labels():
        name, elevation, _, serial = label["instance"]
        key = (name, elevation)
        if key not in cache:
            cache[key] = todays_shapes(json.loads((scenes / name / str(elevation) / "scene.json")
                                                  .read_text(encoding="utf-8")))
        shape = TODAY.get(cache[key].get(serial, "not drawn"), "none")
        kind = shape == label["shape"]
        eye = judged.get(label["frm"], {}).get("verdict") == "pass"
        ok = kind and eye
        passed += ok
        weighted += label["count"] * ok
        total += label["count"]
        print(f"{'PASS' if ok else 'fail'}  {label['frm']:13} label {label['shape']:8} engine {shape:7} "
              f"eye {judged.get(label['frm'], {}).get('verdict', '-')}")
    print(f"score {passed}/{len(labels())} = {100 * passed / len(labels()):.0f}%, "
          f"weighted by how often they stand {100 * weighted / total:.0f}%")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("command", choices=("shoot", "sheets", "score"))
    parser.add_argument("--only", help="comma-separated FRMs (shoot)")
    parser.add_argument("--scenes", type=Path, default=ROOT.parent / "fallout2-ce-3d" / "game" / "mod-cache" / "scene"
                        if not (ROOT / "game").is_dir() else ROOT / "game" / "mod-cache" / "scene")
    args = parser.parse_args()
    if args.command == "shoot":
        shoot(set(args.only.split(",")) if args.only else None)
    elif args.command == "sheets":
        sheets()
    else:
        score(args.scenes)


if __name__ == "__main__":
    main()
