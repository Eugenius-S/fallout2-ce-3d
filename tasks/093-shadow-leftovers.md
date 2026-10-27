---
id: 093
title: Shadow leftovers on corners, an eave strip and a table poking through a wall
milestone: M1
status: todo
owner: none
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The five shadow faults the owner found in the live game after 090 are gone:
a building's corner takes the same shadow as the walls beside it, at every
hour.

## Context

- 090 (PR #126) closed the seams; the owner, 2026-10-27, played it live:
  "already very good, no gaps", and found these, shots in `runs/inbox/`:
  1. `2026-10-27-shadow-leftover-1.png`, `-3.png`: corners of a brick
     building partly ignore the shadow; the corner piece stays lit while
     the walls beside it are shaded.
  2. `-2.png`: a strip on the building's corner (red outline), at the top
     of the corner under the roof's edge.
  3. When the sun moves to the other side, the corner goes dark as a whole.
  4. `-5.png`: the edge of the stuff on a table sticks out through the
     bathhouse wall (in the shot, the boxes and planks by the wall's
     corner).
  5. `-4.png`: the shadow moved onto the left house's corner; the owner
     guesses the corner element is one object, so it is shaded as a whole.
- Faults 1, 3 and 5 look like one cause: a corner is one card or one
  object, so it gets one light value for both faces, or the face of the
  wrong wall. 090 already gives caps and door cards their own wall's side
  of the sun; check how a corner piece is lit against that.
- Find each spot with the fly camera and the dev panel's hour; the 090
  measurement (`tools/` seams metric, the 16 views and 3 fly views)
  stays the yardstick.

## Acceptance

- Before | after in `runs/093-shadow-leftovers/` for each of the five, at
  morning, noon and evening, on the same frames.
- The 090 seam totals do not rise on any view.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

New shadow features, the moon, shadow softness.

## Log
