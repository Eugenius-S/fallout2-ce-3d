---
id: 040
title: The shapes that still fail the eval
milestone: M1
status: doing
owner: claude
suggested: claude
claude-only: no
user-check: yes
depends-on: [037]
reviewed: no
---

## Goal

The eval FRMs that 037 left failing look right from turned views.

## Context

- The design: [docs/2026-10-20-shapes-and-rotation-design.md](../docs/2026-10-20-shapes-and-rotation-design.md);
  its "How the agents work on this" holds.
- `py -3 tools/shapes/eval/eval.py score` lists them; their sheets come from
  `eval.py shoot --only <frm>` and `eval.py sheets` (`build/shapes-eval/`).
- Failing by eye (037's end): firehy (a hole at 270), tbl1000 (the top
  floats, the stem is a static rest card), sstove2 (stretched mirrored
  sides), barrel4 (striped ghost backs), boxes2 (stray pieces), posts (fanned
  planes; label sheet), car5 (a card), cavbrac1 (a plane edge-on). The drum
  ones come from the far halves 033 added (`tools/scene_export/solids.py`).
- Wrong kind only: ss122 and ss108 (box with a back), crate1 (a box).

## Acceptance

- Each fixed FRM passes (`eval.py score`), its sheet before | after in
  `runs/040-shapes-eval-failures/`.
- Regression 16/16, or changed references with before | after.

## Out of scope

New labels (the owner's).

## Log

### 2026-10-20 claude (part 1)
Done: walls.bin version 5 has a third kind of batch, for the game's camera
only (GAME_ONLY), skipped while flying. A drum on its card's depth draws its
near half there, exact for the game's view, and the fly camera gets a whole
drum of its own whose quarters repeat the front quarter mirrored (the near
half's sides by the outline showed a few edge columns stretched: the hydrant
looked hollow). A rest card (a table's stem, flames) is the game's camera's
only; the fly camera sees two crossed upright sheets
(`solids.rest_cross`). firehy, barrel4 and tbl1000 pass: eval 32/40 (80%),
weighted 99% (`runs/040-shapes-eval-failures/score.txt`, sheets
`after-drums-and-stem.jpg`; before in `runs/036-shapes-eval/today-page3.jpg`
and `today-page5.jpg`). Regression 16/16 (klatoxcv-e1 4 pixels, within
tolerance); exporter tests 43/43.
Next: the stove (the donor fits a box), the heap of crates, the notice board,
the car, the cave brace; the bed and the chair as a box with a back; crate1.
Check: `py -3 tools/shapes/eval/eval.py score`.

### 2026-10-20 claude (part 2)
Done: the score reads a box whose far sides rise more than 0.15 units over its
top (the donor's wallU, wallV) as a box with a back: the bed's headboard
stands on its far edge, so ss122 passes. A box label accepts a box with a back
(it is still a box); a box-with-a-back label does not accept a plain box.
Eval 33/40 (82%), weighted 99%.
Left: ss108 (the chair) and crate1 are cards because the donor's fit
(`vendor/fp/props.py`: top fill, bulk, cover) turns down things on legs and
small crates; the stove is a box the label wants as a drum; the heap of
crates, the notice board, the car, the cave brace.
Next: a fitter for a seat on legs with a back (chairs), then crate1.
Check: `py -3 tools/shapes/eval/eval.py score`.
