---
id: 044
title: Shapes eval finds things that vanish from the side by itself
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

The eval flags by script the gross failures seen from turned views: a thing
that shrinks to a sliver or disappears from the side (a card or a plane seen
edge-on, a hole), so that for the 465 FRMs of Klamath (task 039) only the
doubtful ones need a look.

## Context

- Each FRM is shot alone (`-Solo`, no walls or roofs) from 0, 45, 90, 180
  and 270 degrees: `build/shapes-eval/shots/<frm>/<angle>/*.png`
  (`py -3 tools/shapes/eval/eval.py shoot`). The floor is drawn too, and
  other things within one hex.
- The game's view (angle 0) shows the whole thing; a thing that holds its
  shape shows a comparable amount of itself from the other sides.
- Idea to try first: shoot each view twice, with the thing and without it
  (an empty solo: a radius that keeps nothing, or a tile next to it), and
  count the pixels that differ: that is how much of the thing is seen.
  Compare each angle's count with angle 0's.

## Acceptance

- `eval.py score` (or a new `eval.py check`) prints, per FRM, the share of
  the thing seen at each angle against angle 0, and flags an FRM where some
  angle falls under a threshold (start at 0.3, write the chosen one in the
  `Log`).
- On today's eval the flags agree with the by-eye verdicts in
  `verdicts.json` for most FRMs; the `Log` lists where they disagree.

## Out of scope

Replacing the by-eye verdicts; judging what a shape looks like.

## Log
