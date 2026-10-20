"""Writes labels.json from the list below and the inventory's first instance
of each FRM (build/shapes-eval/inventory.json, tools/shapes/inventory.py).
Run once; after the owner's approval labels.json is frozen."""
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]

# frm, shape, big (at least two hexes across or taller than half a wall), note
LABELS = [
    ("eggs7.frm", "flat", False, "small rocks"),
    ("weed01.frm", "crossed", False, ""),
    ("tree8.frm", "crossed", True, ""),
    ("tree7.frm", "crossed", True, ""),
    ("tree9.frm", "crossed", True, ""),
    ("roks2.frm", "flat", False, ""),
    ("bush3.frm", "crossed", False, ""),
    ("weed02.frm", "crossed", False, ""),
    ("weed04.frm", "crossed", False, ""),
    ("maryj02.frm", "crossed", False, "a low bush"),
    ("maryj01.frm", "crossed", False, "a low bush"),
    ("corn5.frm", "crossed", False, ""),
    ("rock09.frm", "box", False, "a heap of rocks"),
    ("roks3.frm", "flat", False, ""),
    ("weed07.frm", "flat", False, "a low mound with weeds"),
    ("roks.frm", "flat", False, ""),
    ("rock10.frm", "box", False, "a heap of rocks"),
    ("drock7.frm", "flat", False, ""),
    ("goop03.frm", "flat", False, ""),
    ("goop04.frm", "flat", False, ""),
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
    ("bvs35.frm", "box", False, "a computer (owner: the stand should be a box)"),
    ("locker6.frm", "box", True, "a locker"),
    ("barrel4.frm", "drum", False, ""),
    ("car5.frm", "box", True, "a wrecked car"),
    ("cab1.frm", "flat", False, "a small flat thing on the floor"),
    ("bigshlf1.frm", "sheet", True, "a big shelf"),
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
