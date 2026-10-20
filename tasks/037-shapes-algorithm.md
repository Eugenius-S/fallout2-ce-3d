---
id: 037
title: Things get their shapes by one algorithm
milestone: M1
status: doing
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
