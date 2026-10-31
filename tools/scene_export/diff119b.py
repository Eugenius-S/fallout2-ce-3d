"""Transition matrix of props rows between two caches: geometry-identical seen flips,
geometry changes, births and deaths. Names resolved from the scene's names table."""
import json
import sys
from collections import Counter
from pathlib import Path

BASE, RULES = Path(sys.argv[1]), Path(sys.argv[2])
MAPS = sys.argv[3].split(",") if len(sys.argv) > 3 else ["klamall"]

def geom_key(row):
    return (round(row[1], 4), round(row[2], 4), row[3], row[4],
            tuple(round(x, 4) for x in row[5:14]), row[14])

for map_ in MAPS:
    for elev in sorted((int(p.name) for p in (BASE / map_).iterdir() if p.is_dir())):
        pa = json.loads((BASE / map_ / str(elev) / "scene.json").read_text(encoding="utf-8"))
        pb = json.loads((RULES / map_ / str(elev) / "scene.json").read_text(encoding="utf-8"))
        names_a, names_b = pa.get("names"), pb.get("names")
        props_a, props_b = pa["props"], pb["props"]
        flips, changed, born, died = Counter(), [], [], []
        pool_b = {}
        for row in props_b:
            pool_b.setdefault(geom_key(row), []).append(row)
        for row in props_a:
            key = geom_key(row)
            if pool_b.get(key):
                pool_b[key].pop()
                continue
            match = [r for k, rs in pool_b.items() if k[:3] == key[:3] for r in rs]  # same place, other geometry
            hit = None
            for cand in match:
                if cand[0] == row[0] or cand[13] == row[13]:
                    hit = cand
                    break
            if hit is not None:
                pool_b[geom_key(hit)].pop()
                changed.append((row, hit))
            else:
                died.append(row)
        for left in [r for rs in pool_b.values() for r in rs]:
            born.append(left)
        for row, hit in changed:
            same_geom = geom_key(row) == geom_key(hit)
            flips[(row[15], hit[15], "geom" if not same_geom else "same")] += 1
        print(f"{map_} e{elev}: {len(props_a)} -> {len(props_b)} rows; flips {dict(flips)}; "
              f"born {len(born)}, died {len(died)}")
        for row, hit in changed:
            if geom_key(row) != geom_key(hit):
                nm = names_a[row[13]] if names_a and row[13] < len(names_a) else row[13]
                print(f"  geom changed {nm!r} at ({row[1]},{row[2]}): seen {row[15]}->{hit[15]}, "
                      f"a {row[7]}->{hit[7]}, b {row[8]}->{hit[8]}, z1 {row[10]}->{hit[10]}, board {row[3]}->{hit[3]}")
        for row in born:
            nm = names_b[row[13]] if names_b and row[13] < len(names_b) else row[13]
            print(f"  born seen={row[15]} {nm!r} at ({row[1]},{row[2]}) board {row[3]}")
        for row in died:
            nm = names_a[row[13]] if names_a and row[13] < len(names_a) else row[13]
            print(f"  died seen={row[15]} {nm!r} at ({row[1]},{row[2]}) board {row[3]}")
