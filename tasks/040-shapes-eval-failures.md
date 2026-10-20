---
id: 040
title: The shapes that still fail the eval
milestone: M1
status: todo
owner: none
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
