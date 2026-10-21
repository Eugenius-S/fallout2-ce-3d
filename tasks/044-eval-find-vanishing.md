---
id: 044
title: Shapes eval finds things that vanish from the side by itself
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

### 2026-10-21 glm
Done: `eval.py shoot` now shoots every view twice: the thing at solo radius 1,
then an empty twin at radius 0 on the same tile with the same camera (radius 0
keeps only things standing on the tile itself, so the twin is the spot without
the thing); twins go to `build/shapes-eval/empties/`. New `eval.py check
[--threshold T]` counts the thing's pixels per angle (strongest channel delta
8 or more) against the game's own view (angle 0) and flags an FRM whose worst
side share falls under the threshold; it prints each FRM's 45/90/180/270
shares, the summary, and where the flags meet the eye verdicts.
Threshold chosen: 0.3. At 0.4 five FRMs flag and four of them carry pass
verdicts; at 0.3 one. Flats baked into the floor measure 0 px (the empty solo
cannot remove them) and are skipped: 14 of 40, among them cavbrac1, whose eye
fail is therefore not measurable by this check.
Today's eval: 27 measurable, 25 agree with verdicts.json. Disagreements:
sstove2.frm flagged (180 degrees shows 0.12 of the game view) but eye pass;
boxes2.frm not flagged (worst share 0.38) but eye fail, for stray pieces
rather than a vanishing side.
Next: nothing.
Check: `eval.py shoot` (40 FRMs, 370 paired stops) then `eval.py check`:
the output above this entry in the run log; re-running check is deterministic
over the saved shots.
