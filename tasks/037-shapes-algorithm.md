---
id: 037
title: Things get their shapes by one algorithm
milestone: M1
status: done
owner: claude
suggested: claude
claude-only: no
user-check: yes
depends-on: [036]
reviewed: no
---

## Goal

Every static thing gets its shape (card, flat, plane on an axis, sheet,
crossed sheets, box, drum, box with a back, several boxes) by the rules of the
design, and at least 70% of the eval set passes.

## Context

- The design: [docs/2026-10-20-shapes-and-rotation-design.md](../docs/2026-10-20-shapes-and-rotation-design.md). Read it first; its "How the agents work on
  this" holds for this task.
- Today's shapes: solids (`tools/scene_export/solids.py`, from the donor's
  `props.py` fit), flats (`baked_sidecar.py`, 035), cards (live objects).
- The game's view must stay pixel for pixel: every shape is textured by
  projecting the sprite from the game camera.

## Acceptance

- Eval score 70% or more (036's command), the score of every iteration in
  the `Log`.
- Regression 16/16, or changed references each with a before | after image.
- Contact sheets before | after for the owner's spots in
  `runs/037-shapes-algorithm/`.
- Exporter tests for each rule.

## Out of scope

The rotation option (038). Hand-written exceptions beyond the eval set (039).

## Log

### 2026-10-20 claude (done)
Done: two rules in `tools/scene_export/baked_sidecar.py` (baked.bin version
4), each only under the fly camera, so the game's view stays pixel for pixel:
doors and gates (scene.doors) lie on their wall's plane (PLANE: each card
corner slides along the game camera's ray onto it, `onFlyPlane`); cards no
more than 20 px tall and wider than tall lie flat (FLY_FLAT). Laying them flat
in the game's view too changed 8 regression views, so it is flight only.
Eval 25/40 -> 29/40 (72%), weighted 98% (`runs/037-shapes-algorithm/score.txt`,
after sheets `after-doors-gate-cabbage.jpg`, before in
`runs/036-shapes-eval/today-page4.jpg` and `today-page5.jpg`).
The solo dev view keeps the hero now: without it the fly camera does not take
off. weed07 is labelled a card, as the other plants (it read right as one).
Regression 16/16 unchanged; exporter tests 42/42.
Left: the failures listed in task 040.
Next: 040 or 038.
Check: `py -3 tools/shapes/eval/eval.py score` (on the worktree's export).
