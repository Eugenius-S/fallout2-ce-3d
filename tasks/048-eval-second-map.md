---
id: 048
title: Shapes eval gets things from a second town, not only Klamath
milestone: M1
status: done
owner: glm
suggested: gpt
claude-only: no
user-check: yes
depends-on: [041]
reviewed: no
---

## Goal

The shapes algorithm is checked outside Klamath, so its 88% is not a fit to
one town.

## Context

- The eval: `tools/shapes/eval/` (`labels.json`, `make_labels.py`,
  `eval.py`), the rules in `docs/2026-10-20-shapes-and-rotation-design.md`
  ("The eval set").
- Take the Den: export its town maps as a comma list (`export.py --maps
  denbus1,denbus2,denres1`; check the names in `master.dat`), add a `den`
  group next to `klamath` in `export.py`, pick the 15 most frequent static FRMs not already in the labels, plus any
  big ones (a building part, a car, a large machine).
- Label them by the same rules as the Klamath labels (plants and low rocks are
  cards, shelves and crates boxes); mark each label with its town.
- The owner may change any label later; that changes nothing in this task.

## Acceptance

- `labels.json` has 15 or more Den labels; `eval.py shoot` and `score` run
  on them.
- The `Log` holds the Klamath score and the Den score apart, and the failing
  Den FRMs.
- No change to the algorithm in this task: the failures become a new task.

## Log

### 2026-10-24 glm
Done: the shapes eval has Den labels, so the 88% is no longer a fit to one
town. export.py grew the "den" group (denbus1, denbus2, denres1, the names
checked in master.dat); tools/shapes/inventory.py grew --group; the Den
inventory is 298 FRMs over 5836 things (build/shapes-eval/inventory-den.json,
a copy in runs/). 18 Den labels by the Klamath rules (plants and low heaps
cards, shelves and crates boxes, chairs and couches boxes with a back,
barrels and tire stacks drums, doors planes; the art of each candidate dumped
and looked at first: runs/048-eval-second-map/den-frm-contact.png), each
marked with its town in labels.json (make_labels takes the row's town and its
instance from that town's inventory). eval.py shoot and score run on them
(the score reads the Den scenes' scene.json and baked.bin as for Klamath).
Scores apart (runs/048-eval-second-map/score.txt): Klamath 35/40 (88%) as
frozen; Den by shape 13/18 (72%), the eye part is the owner's (no Den
verdicts yet; every Den row prints eye "-"). The failing Den FRMs: ss112 and
ss118 (chairs the donor leaves as cards), tirs002 (a tire stack the engine
boxes with a back, labeled a drum), cac2000 (a cactus the engine solids on
some instances, labeled a card), char06 (a couch the engine boxes, labeled
box with a back; the owner may flip the label instead). car1, the big car the
task asked for, passes as a box. No change to the algorithm.
Check: py -3 tools/shapes/eval/eval.py score --scenes build/scene-cache
(58 labels; the sheet page of the Den labels in
runs/048-eval-second-map/den-sheets-page.png).
Next: the owner judges the Den labels (user-check: yes) and the failures
become a new task. Note: the Klamath FRMs' raw solo shots under
build/shapes-eval were re-baselined during this task (a stale test instance
broke a shoot run); the frozen verdicts.json is untouched (restored from
git), and the score does not read the sheets; a Klamath re-shoot before the
owner's next eye pass rebuilds them.
