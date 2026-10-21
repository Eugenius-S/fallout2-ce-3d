---
id: 043
title: Shapes eval shows only the sheets that changed
milestone: M1
status: done
owner: glm
suggested: glm
claude-only: no
user-check: no
depends-on: [041]
reviewed: yes
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

### 2026-10-21 glm
Done: `eval.py sheets` keeps the run's sheets in `build/shapes-eval/previous/`
and compares against them. Tolerance: a pixel counts when its strongest
channel differs by 8 or more, and a sheet is changed past 32 such pixels
(`DIFF_CHANNEL`, `DIFF_PIXELS` in eval.py); the shots are deterministic, so
this only absorbs trivial jitter. Changed FRMs go to
`build/shapes-eval/changed.png`, each as previous over new behind a red
divider, their names print, and their verdicts get `"stale": true` in
verdicts.json (an entry with no verdict is left alone). `eval.py score` shows
such an FRM's eye as `stale`, fails it, and the summary says how many are
stale. Nothing changed: it prints that (and the count of first baselines) and
writes no `changed.png`, removing a stale one.
Next: nothing.
Check: ran sheets twice on the eggs7 shots: first run baselines, second says
"unchanged" and writes no changed.png; a faked 90 degree shot made
changed.png with eggs7.frm previous over new, marked its verdict stale, and
score printed "1 stale, not judged". The faked state is removed.
