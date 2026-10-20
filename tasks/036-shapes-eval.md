---
id: 036
title: Eval set for the shapes of things
milestone: M1
status: done
owner: claude
suggested: claude
claude-only: no
user-check: yes
depends-on: [034]
reviewed: no
---

## Goal

A fixed set of about 40 Klamath FRMs with labels and contact sheets from
four turned views, and a score that says how many of them look right.

## Context

- The design: [docs/2026-10-20-shapes-and-rotation-design.md](../docs/2026-10-20-shapes-and-rotation-design.md). Read it first; its "How the agents work on
  this" holds for this task.
- The 20 FRMs that stand most often on Klamath's maps plus 20 by kind and by
  the owner's reports (`runs/035-donor-things/owner-*`), as the design lists.
- Turned views can be taken with the fly camera today:
  `tools/dev-snapshot.ps1 -HeroTile <tile> -ViewTile <tile> -FlyTurn <deg>
  -FlyPitch <the game camera's pitch> -FlyDistance <hexes>`.

## Acceptance

- `tools/shapes/eval/labels.json`: every eval FRM with its expected shape
  and, for solids, its rough size; one instance (map, elevation, tile) each.
  The owner approved the labels (say so in the `Log`).
- One command makes every FRM's contact sheet (0, 90, 180, 270 and 45
  degrees) into `build/shapes-eval/` and prints the score: the labelled shape
  matches (script), plus the by-eye verdicts kept in
  `tools/shapes/eval/verdicts.json`.
- The score of today's shapes in the `Log`, with a few sheets in
  `runs/036-shapes-eval/`.

## Out of scope

Changing any shape (037).

## Log

### 2026-10-20 claude
Done: `tools/shapes/inventory.py` counts Klamath's static things by FRM (465
FRMs, 10226 things the engine draws; plants, rocks and goo are most of them)
and says how the engine draws each today. `tools/shapes/eval/labels.json`
(from `make_labels.py`): 40 FRMs, the 20 most frequent (the two black
map-edge masks swapped for corn5 and rock09) and 20 by kind and the owner's
reports. `--dev-solo=TILE,R` / `dev-snapshot.ps1 -Solo R` draws only the
things within R hexes of the view tile (no walls, roofs or hero), so a thing
is seen alone from every side. `tools/shapes/eval/eval.py` shoots, makes the
sheets and scores. The engine gives 17 of 40 their labelled shape today.
Next: shoot all, look at the sheets, write verdicts.json, the score.
Check: `py -3 tools/shapes/eval/eval.py score`.

### 2026-10-20 claude (done)
Done: every eval FRM shot alone from 0, 45, 90, 180 and 270 degrees in full
daylight (`eval.py shoot`, about 15 s a view), the sheets looked at, verdicts
in `tools/shapes/eval/verdicts.json`. Today's score: 25 of 40 (62%); weighted
by how often the 40 stand, 96%, because plants and rocks are most of Klamath
and they already read right (`runs/036-shapes-eval/today-score.txt`, sheets
`today-page1..5.jpg`).
Labels changed after the first sheets, before the owner's approval: plants and
low heaps of rocks are cards (turned to the camera they read right from every
side; crossed sheets would show their cross from above), the big shelf is a
box (it reads right as one). klatoxcv has no eval FRM: a fight starts there at
once and the fly camera lands, so goo and the lab computer were swapped for
weed03, drock4 and desk1.
Failing by eye: the notice board (fanned planes), the hydrant (a hole), the
round table (the top floats), the door and the gate (they turn with the
camera), the stove (stretched sides), the heap of crates (stray pieces), the
barrel stack (striped ghost backs), the car (a card), the cave brace (a plane
edge-on). Wrong kind only: the bed and the chair (box+back), crate1 (a card),
weed07 and cab1 (cards that should lie flat).
Owner: approve the labels (`tools/shapes/eval/labels.json`, notes say which
are your reports); until then 037 may start, the labels stay as they are.
Next: 037.
Check: `py -3 tools/shapes/eval/eval.py score`; `eval.py shoot` then `sheets`
for new views.
