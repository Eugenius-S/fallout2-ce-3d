"""Writes labels.json from the list below and the inventory's first instance
of each FRM (build/shapes-eval/inventory.json, tools/shapes/inventory.py).
Run once; after the owner's approval labels.json is frozen. Nothing from
klatoxcv: a fight starts there at once and the fly camera lands.
The Den rows (task 048) take their instances from inventory-den.json
(tools/shapes/inventory.py --group den); their labels follow the Klamath
rules: plants and low heaps are cards, shelves and crates boxes, chairs and
couches boxes with a back, barrels and tire stacks drums."""
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]

# Plants and low heaps are cards: turned to the camera they read right from
# every side (the first 18 sheets, 2026-10-20); crossed sheets would show
# their cross from above.
# frm, shape, big (at least two hexes across or taller than half a wall), note[, town]
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
    ("weed07.frm", "card", False, "a low mound with weeds"),
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
    ("weed26.frm", "card", False, "", "den"),
    ("weed24.frm", "card", False, "", "den"),
    ("barrel.frm", "drum", False, "the burning barrel", "den"),
    ("ss112.frm", "box+back", False, "a chair", "den"),
    ("eggs6.frm", "flat", False, "small rocks", "den"),
    ("doorb05.frm", "plane", True, "a door, as doors", "den"),
    ("weed25.frm", "card", False, "", "den"),
    ("brl2000.frm", "drum", False, "a metal barrel", "den"),
    ("eggs5.frm", "flat", False, "small rocks", "den"),
    ("weed09.frm", "card", False, "a low mound of rocks", "den"),
    ("brl1000.frm", "drum", False, "a metal barrel", "den"),
    ("car1.frm", "box", True, "a car, the big one the task asked for", "den"),
    ("tirs002.frm", "drum", False, "a stack of tires", "den"),
    ("cac2000.frm", "card", False, "a cactus", "den"),
    ("char05.frm", "box+back", False, "a couch", "den"),
    ("char06.frm", "box+back", False, "a couch", "den"),
    ("ss118.frm", "box+back", False, "a chair", "den"),
    ("weed08.frm", "card", False, "junk", "den"),
]


def main():
    inventories = {"klamath": {row["frm"]: row for row in
                               json.loads((ROOT / "build/shapes-eval/inventory.json").read_text())}}
    den = ROOT / "build/shapes-eval/inventory-den.json"
    if den.is_file():
        inventories["den"] = {row["frm"]: row for row in json.loads(den.read_text())}
    rows = []
    for label in LABELS:
        frm, shape, big, note = label[0], label[1], label[2], label[3]
        town = label[4] if len(label) > 4 else "klamath"
        row = inventories[town][frm]
        rows.append({"frm": frm, "name": row["name"], "shape": shape, "big": big, "note": note,
                     "town": town, "count": row["count"], "instance": row["instances"][0]})
    (HERE / "labels.json").write_text(json.dumps(rows, indent=1) + "\n", encoding="utf-8")
    print(len(rows), "labels")


if __name__ == "__main__":
    main()
