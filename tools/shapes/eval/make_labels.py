"""Writes labels.json from the list below and the inventory's first instance
of each FRM (build/shapes-eval/inventory.json, tools/shapes/inventory.py).
Run once; after the owner's approval labels.json is frozen. Nothing from
klatoxcv: a fight starts there at once and the fly camera lands."""
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]

# Plants and low heaps are cards: turned to the camera they read right from
# every side (the first 18 sheets, 2026-10-20); crossed sheets would show
# their cross from above.
# frm, shape, big (at least two hexes across or taller than half a wall), note
LABELS = [
    ("eggs7.frm", "flat", False, "small rocks"),
    ("weed01.frm", "card", False, ""),
    ("tree8.frm", "card", True, ""),
    ("tree7.frm", "card", True, ""),
    ("tree9.frm", "card", True, ""),
    ("roks2.frm", "flat", False, ""),
    ("bush3.frm", "card", False, ""),
    ("weed02.frm", "card", False, ""),
    ("weed04.frm", "card", False, ""),
    ("maryj02.frm", "card", False, "a low bush"),
    ("maryj01.frm", "card", False, "a low bush"),
    ("corn5.frm", "card", False, ""),
    ("rock09.frm", "card", False, "a low heap of rocks"),
    ("roks3.frm", "flat", False, ""),
    ("weed07.frm", "flat", False, "a low mound with weeds"),
    ("roks.frm", "flat", False, ""),
    ("rock10.frm", "card", False, "a low heap of rocks"),
    ("drock7.frm", "flat", False, ""),
    ("weed03.frm", "card", False, ""),
    ("drock4.frm", "flat", False, ""),
    ("posts.frm", "sheet", False, "the notice board (owner: crumpled from the air)"),
    ("firehy.frm", "drum", False, "the hydrant (owner: a hole)"),
    ("tbl2000.frm", "drum", False, "a cable spool"),
    ("tbl1000.frm", "drum", False, "the round table (owner: its stem is a card)"),
    ("doorb06.frm", "plane", True, "a door (owner: stays on its wall, never turns)"),
    ("gate.frm", "plane", False, "a fence gate (owner: as doors)"),
    ("sstove2.frm", "drum", False, "the stove (owner: its top flickers)"),
    ("ss122.frm", "box+back", True, "the bed (owner: an extra face; the headboard on its plane)"),
    ("bed3.frm", "flat", False, "a sleeping bag"),
    ("ss108.frm", "box+back", False, "a chair (owner: a box and a back)"),
    ("char03.frm", "box", False, "a low seat"),
    ("crate1.frm", "box", False, "a crate"),
    ("boxes2.frm", "boxes", False, "a heap of crates (owner: a stray piece)"),
    ("desk1.frm", "box", False, "a desk"),
    ("locker6.frm", "box", True, "a locker"),
    ("barrel4.frm", "drum", False, ""),
    ("car5.frm", "box", True, "a wrecked car"),
    ("cab1.frm", "flat", False, "a small flat thing on the floor"),
    ("bigshlf1.frm", "box", True, "a big shelf"),
    ("cavbrac1.frm", "sheet", True, "a cave brace: a beam on posts"),
]


def main():
    inventory = {row["frm"]: row for row in json.loads((ROOT / "build/shapes-eval/inventory.json").read_text())}
    rows = []
    for frm, shape, big, note in LABELS:
        row = inventory[frm]
        rows.append({"frm": frm, "name": row["name"], "shape": shape, "big": big, "note": note,
                     "count": row["count"], "instance": row["instances"][0]})
    (HERE / "labels.json").write_text(json.dumps(rows, indent=1) + "\n", encoding="utf-8")
    print(len(rows), "labels")


if __name__ == "__main__":
    main()
