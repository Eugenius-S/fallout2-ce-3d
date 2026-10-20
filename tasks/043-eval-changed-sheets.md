---
id: 043
title: Shapes eval shows only the sheets that changed
milestone: M1
status: todo
owner: none
suggested: glm
claude-only: no
user-check: no
depends-on: [041]
reviewed: no
---

## Goal

After a change to the shapes, whoever judges the eval looks only at the
contact sheets that changed since the last run, not at all of them: looking
at pictures is the dearest part of the eval.

## Context

- `py -3 tools/shapes/eval/eval.py shoot` writes each FRM's views to
  `build/shapes-eval/shots/<frm>/<angle>/`; `eval.py sheets` builds
  `build/shapes-eval/sheets/<frm>.png` and pages of 8.
- The by-eye verdicts are in `tools/shapes/eval/verdicts.json`.
- Views of an unchanged thing come out pixel for pixel the same (the test
  instance is deterministic); a small tolerance still helps (say a few
  pixels).

## Acceptance

- `eval.py sheets` keeps the previous sheets (for example in
  `build/shapes-eval/previous/`) and writes `build/shapes-eval/changed.png`:
  only the changed FRMs, each as previous over new; it prints their names.
- An FRM whose sheet changed has its verdict marked stale (for example
  `"stale": true` in verdicts.json) until someone judges it again; `score`
  counts a stale verdict as not judged and says so.
- Nothing changed: it says so and writes no `changed.png`.

## Out of scope

Judging the sheets by script (task 044).

## Log
