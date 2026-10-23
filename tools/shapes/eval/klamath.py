"""The whole-Klamath shapes pass (task 039, docs/2026-10-20-shapes-and-rotation-design.md).

    py -3 tools/shapes/eval/klamath.py plan                # what to shoot: build/shapes-klamath/plan.json
    py -3 tools/shapes/eval/klamath.py shoot [--only a.frm,b.frm] [--map name:elev]
    py -3 tools/shapes/eval/klamath.py check               # turned views that lose the thing: flags.json
    py -3 tools/shapes/eval/klamath.py sheets [--only ...] # contact sheets for the eye
    py -3 tools/shapes/eval/klamath.py score               # the weighted Klamath score

Cards and flats never turn to the camera, so they pass by design and are not
shot; the plan covers every FRM the engine ever draws as a solid (a box, a
drum, a box with a back, a panel, a part of a solid), one instance each, seen
alone from 0, 90, 180 and 270 degrees in one game run per map elevation
(a tour, as the 40-FRM eval does). check counts the thing's pixels per view
over the shot's empty twin (solo radius 0) and flags a view that keeps under
the threshold share of the game's own view. score weighs each FRM by how
often it stands: cards and flats pass, a solid passes when check does not
flag it and its verdict in verdicts.json (this folder's, written while
looking at its sheet) is not "fail"; not-yet-judged solids report separately.
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
OUT = ROOT / "build" / "shapes-klamath"
ANGLES = (0, 90, 180, 270)
CARD_LIKE = {"card", "flat", "card (donor: crossed)", "card (door)"}

sys.path.insert(0, str(ROOT / "tools" / "scene_export"))
sys.path.insert(0, str(ROOT / "tools" / "scene_export" / "vendor"))
sys.path.insert(0, str(HERE.parent))

from eval import THING_PIXELS, thing_pixels  # noqa: E402
from export import MAP_GROUPS  # noqa: E402

GAME_ROOT = ROOT.parent / "fallout2-ce-3d" / "game"


def rows():
    path = ROOT / "build" / "shapes-eval" / "inventory.json"
    if not path.is_file():
        raise SystemExit("run tools/shapes/inventory.py first")
    return json.loads(path.read_text(encoding="utf-8"))


def plan():
    gf_files = {}

    def frm_size(frm):
        """(width, height) of the art's first frame, for the shot distance."""
        if frm not in gf_files:
            from export import ReadOnlyGameFiles
            from f2lib.frm import Frm
            gf = ReadOnlyGameFiles(GAME_ROOT)
            try:
                for lst in ("art/scenery/scenery.lst", "art/misc/misc.lst", "art/items/items.lst"):
                    names = gf.read(lst).decode("latin-1").splitlines()
                    names = [line.strip().lower() for line in names]
                    if frm in names:
                        size = Frm.from_bytes(gf.read(f"{lst.rsplit('/', 1)[0]}/{frm}")).size(0, 0)
                        gf_files[frm] = size
                        break
                else:
                    gf_files[frm] = (0, 0)
            finally:
                gf.close()
        return gf_files[frm]

    entries = []
    for row in rows():
        if set(row["today"]) <= CARD_LIKE:
            continue  # cards and flats never turn: decent by design
        name, elevation, tile, serial, fid = row["instances"][0]
        width, height = frm_size(row["frm"])
        entries.append({
            "frm": row["frm"], "name": row["name"], "count": row["count"], "today": row["today"],
            "instance": [name, elevation, tile, serial, fid],
            "big": max(width, height) > 120,
        })
    entries.sort(key=lambda e: (e["instance"][0], e["instance"][1], e["frm"]))
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "plan.json").write_text(json.dumps(entries, indent=1), encoding="utf-8")
    print(f"{len(entries)} solid FRMs to shoot; {OUT / 'plan.json'}")


def entries(only):
    path = OUT / "plan.json"
    if not path.is_file():
        raise SystemExit("run `klamath.py plan` first")
    data = json.loads(path.read_text(encoding="utf-8"))
    return [e for e in data if not only or e["frm"] in only]


def copy_shot(shot, frm, angle, empty):
    tree = OUT / ("empties" if empty else "shots") / frm / str(angle)
    if tree.exists():
        shutil.rmtree(tree)
    tree.mkdir(parents=True)
    shutil.copy(shot, tree / shot.name)


def shoot_single(groups):
    """One game run per view: for maps whose entrance starts a fight (the
    tour then waits on a blocking state forever)."""
    for (name, elevation), group in sorted(groups.items()):
        for entry in group:
            distance = 6 if entry["big"] else 3.5
            tile = entry["instance"][2]
            for angle in ANGLES:
                for solo, frm, empty in ((1, entry["frm"], False), (0, entry["frm"], True)):
                    folder = OUT / "batch" / f"{name}-{elevation}-single"
                    if folder.exists():
                        shutil.rmtree(folder)
                    folder.mkdir(parents=True)
                    command = (f"& '{ROOT / 'tools' / 'dev-snapshot.ps1'}' -Map {name} -Elevation {elevation} "
                               f"-ViewTile {tile} -HeroTile {tile} -Solo {solo} -Turn {angle // 90} "
                               f"-Ambient 65536 -OutputDirectory '{folder}' -TimeoutSeconds 120")
                    run = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", command],
                                         capture_output=True, text=True, errors="replace")
                    shots = sorted(folder.glob("*.png"))
                    if run.returncode != 0 or not shots:
                        print(f"{name} e{elevation} {frm} {angle} solo {solo}: failed", flush=True)
                        continue
                    copy_shot(shots[-1], frm, angle, empty)
            print(f"{name} e{elevation} {entry['frm']} done", flush=True)


def shoot(only, map_filter, single=False):
    groups = {}
    for entry in entries(only):
        groups.setdefault(tuple(entry["instance"][:2]), []).append(entry)
    if single:
        if map_filter:
            groups = {k: v for k, v in groups.items() if f"{k[0]}:{k[1]}" == map_filter}
        shoot_single(groups)
        return
    for (name, elevation), group in sorted(groups.items()):
        if map_filter and f"{name}:{elevation}" != map_filter:
            continue
        # A group already complete from an earlier run stays as it is.
        if all(sorted((OUT / "shots" / e["frm"] / str(angle)).glob("*.png"))
               and sorted((OUT / "empties" / e["frm"] / str(angle)).glob("*.png"))
               for e in group for angle in ANGLES):
            print(name, elevation, len(group), "FRMs already shot", flush=True)
            continue
        stops, order = [], []
        for entry in group:
            distance = 6 if entry["big"] else 3.5
            tile = entry["instance"][2]
            for angle in ANGLES:
                stops.append(f"{name}:{elevation}:{tile}:1:{angle}:{distance}")
                order.append((entry["frm"], angle, False))
                stops.append(f"{name}:{elevation}:{tile}:0:{angle}:{distance}")
                order.append((entry["frm"], angle, True))
        folder = OUT / "batch" / f"{name}-{elevation}"
        if folder.exists():
            shutil.rmtree(folder)
        folder.mkdir(parents=True)
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


def verdict_data():
    path = HERE / "klamath-verdicts.json"
    return json.loads(path.read_text(encoding="utf-8")) if path.is_file() else {}


def save_verdicts(data):
    (HERE / "klamath-verdicts.json").write_text(json.dumps(data, indent=1, ensure_ascii=False) + "\n",
                                                encoding="utf-8")


def check(threshold):
    flags = {}
    for entry in entries(None):
        frm = entry["frm"]
        shares = {}
        measurable = True
        for angle in ANGLES:
            shots = sorted((OUT / "shots" / frm / str(angle)).glob("*.png"))
            empties = sorted((OUT / "empties" / frm / str(angle)).glob("*.png"))
            if not shots or not empties:
                shares[angle] = None
                continue
            pixels = thing_pixels(shots[-1], empties[-1])
            shares[angle] = pixels
        base = shares.get(0)
        if base is None or base < THING_PIXELS:
            flags[frm] = {"why": "not measurable", "shares": shares}
            continue
        lost = {angle: round(shares[angle] / base, 2) for angle in ANGLES
                if angle and shares.get(angle) is not None and shares[angle] / base < threshold}
        if lost:
            flags[frm] = {"why": "loses the thing from a side", "shares": shares, "shares_of_view0": lost}
    (OUT / "flags.json").write_text(json.dumps(flags, indent=1), encoding="utf-8")
    print(f"{len(flags)} flagged of {len(entries(None))} shot; {OUT / 'flags.json'}")


def sheets(only):
    from PIL import Image, ImageDraw
    cell_w, cell_h, head = 300, 240, 18
    for entry in entries(only):
        frm = entry["frm"]
        row = Image.new("RGB", (cell_w * len(ANGLES), cell_h + head), (24, 24, 24))
        draw = ImageDraw.Draw(row)
        verdict = verdict_data().get(frm, {})
        draw.text((4, 2), f"{frm}  {entry['name']}  count {entry['count']}  today {entry['today']}  "
                  f"verdict: {verdict.get('verdict', '-')} {verdict.get('why', '')}", fill=(255, 230, 0))
        for column, angle in enumerate(ANGLES):
            shots = sorted((OUT / "shots" / frm / str(angle)).glob("*.png"))
            if not shots:
                continue
            image = Image.open(shots[-1]).convert("RGB")
            w, h = image.size
            image = image.crop((int(w * 0.28), int(h * 0.12), int(w * 0.72), int(h * 0.82)))
            image.thumbnail((cell_w - 4, cell_h - 4))
            row.paste(image, (column * cell_w + 2, head + 2))
            draw.text((column * cell_w + 4, head + 4), f"{angle}", fill=(255, 255, 255))
        (OUT / "sheets").mkdir(parents=True, exist_ok=True)
        row.save(OUT / "sheets" / f"{frm}.png")
    pages = sorted((OUT / "sheets").glob("*.png"))
    for start in range(0, len(pages), 8):
        part = pages[start:start + 8]
        first = Image.open(part[0]).convert("RGB")
        page = Image.new("RGB", (first.width, sum(Image.open(p).height for p in part)))
        y = 0
        for p in part:
            page.paste(Image.open(p).convert("RGB"), (0, y))
            y += Image.open(p).height
        page.save(OUT / f"page{start // 8 + 1}.png")
    print(f"{len(pages)} sheets, pages in {OUT}")


def score():
    data = rows()
    total = sum(row["count"] for row in data)
    flags = json.loads((OUT / "flags.json").read_text(encoding="utf-8")) if (OUT / "flags.json").is_file() else {}
    judged = verdict_data()
    passed = unjudged_weight = unjudged = failed = 0
    failures = []
    for row in data:
        if set(row["today"]) <= CARD_LIKE:
            passed += row["count"]
            continue
        entry = judged.get(row["frm"], {})
        verdict = entry.get("verdict")
        flagged = flags.get(row["frm"])
        if verdict == "fail":
            failed += row["count"]
            failures.append(f"{row['frm']} (eye: {entry.get('why', '')})")
        elif flagged and flagged["why"] == "loses the thing from a side" and verdict != "pass":
            failed += row["count"]
            failures.append(f"{row['frm']} ({flagged['why']}: {flagged.get('shares_of_view0', '')})")
        elif verdict == "pass":
            passed += row["count"]
        elif flagged:  # not measurable: things the solo cannot separate, as flats
            passed += row["count"]
        else:
            unjudged += 1
            unjudged_weight += row["count"]
    print(f"weighted score {100 * passed / total:.0f}% pass, {100 * failed / total:.1f}% fail, "
          f"{100 * unjudged_weight / total:.1f}% unjudged ({unjudged} FRMs)")
    if failures:
        print("failures: " + ", ".join(failures))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("command", choices=("plan", "shoot", "check", "sheets", "score"))
    parser.add_argument("--only", help="comma-separated FRMs")
    parser.add_argument("--map", help="one map elevation, name:elevation (shoot)")
    parser.add_argument("--single", action="store_true", help="shoot one game run per view (maps that fight at their entrance)")
    parser.add_argument("--threshold", type=float, default=0.3, help="check: the share under which a side flags")
    args = parser.parse_args(argv)
    only = set(args.only.split(",")) if args.only else None
    if args.command == "plan":
        plan()
    elif args.command == "shoot":
        shoot(only, args.map, args.single)
    elif args.command == "check":
        check(args.threshold)
    elif args.command == "sheets":
        sheets(only)
    else:
        score()


if __name__ == "__main__":
    main()
