---
id: 125
title: Every multi-view box's far sides take the art's own back views
milestone: M1
status: doing
owner: glm
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: [124]
reviewed: no
---

## Goal

124 gave the carve's far sides the art's own back views; the plain-fit
boxes kept the mirror. The arts do not distinguish: a plain-fit cola
machine or truck seen from behind in the fly camera shows its front view
mirrored, text and all, while its opposite slot holds a real rear drawing.

## Context

- The recon (runs/_124_recon/widen125.py) counted the population on 20
  maps: 333 box rows without a back view whose art holds a real, distinct
  opposite slot - truck2 92, truck3 57, car3 36, car1 32, car5 24,
  cac2000 16, CAR4b 23, CAR2b 31, cola 13, CAR1b 4, SHELF002/003/006 5.
- The machinery is already generic: solids.solid_faces reads row[17] of
  any box row; back_sprite's gates (real opposite, BACK_COVER coverage)
  decide per placement; the atlas pins any sprite the rows name beyond
  their near view. The emit site gated the compose on the carve flag
  alone.

## Acceptance

- Fly pairs of plain-fit multi-view things from behind (a cola machine, a
  shelf, a seen=0 truck): the far sides show the arts' own back drawings,
  no see-through texels; the drums keep their clip planes and take no
  back view.
- The game's views stay pixel for pixel on the regression spots; ctest
  passes; the whole-game sweep builds.

## Out of scope

Drums (a drum is never a box; their row 16 keeps the clip planes), the
engine, arts whose opposite slot repeats the near view or is the
placeholder card (the gates keep the mirror).

## Log

### 2026-11-01 glm: the recon, the population is real

widen125.py over 20 maps: 333 box rows (see the table above; seen values
span 0 / 1 / 2 - game-visible solids, keep_apart's fly copies, and the
game-only cut originals). The art-level check only: the coverage gate
trims further (car3 rot 0 measured 0.25 in 124's recon, TRUCK2 0.38 one
way). The emit change is one condition plus one pinning line.
