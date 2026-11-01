---
id: 125
title: Every multi-view box's far sides take the art's own back views
milestone: M1
status: done
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

### 2026-11-01 claude review
Done: merged task 124 and current main; exporter tests 88 OK; ctest 18/18 (fresh build of the stack with main). Regression at this tip (124 and 125 included): the same per-spot changed-pixel counts as the 121 run (entrance 776 ... klatrap 0), so the back views add no pixel to the game's views; those remaining deltas are 121's thin-gap fill. Fly tour of the whole stack against current main: 38 of 43 spots changed (121's fill at pinhole scale; Klamath holds no multi-view box a back view would change), runs/125-widen-back-views/tour-changed.jpg. Not verified: the whole-game build sweep (taken from glm's log).
Check: runs/125-widen-back-views/.

### 2026-11-01 glm: the recon, the population is real

widen125.py over 20 maps: 333 box rows (see the table above; seen values
span 0 / 1 / 2 - game-visible solids, keep_apart's fly copies, and the
game-only cut originals). The art-level check only: the coverage gate
trims further (car3 rot 0 measured 0.25 in 124's recon, TRUCK2 0.38 one
way). The emit change is one condition plus one pinning line.

### 2026-11-01 glm: the widening landed, the gates did the trimming

The emit site now asks back_sprite for every scenery box whose fit is not
a drum (carve_art hoisted out of the carve-only branch; it caches per
art, so the plain-fit rows' ask costs a dict look). The row layout is
124's (the back at 17 behind an empty 16), the pinning moved to the emit
site: a back sprite is never a row's near view, and the game's camera
culls away faces, so it pins to the fly page whatever the row's seen
value - the seen==1 branch's own back pinning became redundant and went.
The keep_apart cut copies carry the row whole (list(row)), so the fly
copies of cut solids keep their backs. Items (the animating containers'
machinery) are out of scope: frames stays None for non-scenery, their
rows keep their shape.

The recon's art-level 333 rows trimmed to 86 rows carrying backs on the
20 surveyed maps - the coverage gate did real work: truck2 (0.38 one
way), car3 rot 0 (0.25), cac2000, the shelf variants SHELF002 / 006 and
most of TRUCK3 stay on the mirror. What passed: CAR4b (34 rows across
seen 0 / 1 / 2), car5 20, cola 13, CAR2b 13, CAR1b 4, SHELF003 1.

Checks: the exporter unittests 87 (the compose's own tests unchanged;
the emit gate is one condition, exercised by the survey and the
regression); ctest 18/18; the 16 regression spots are PIXEL-IDENTICAL to
the 123-rebar run's captures (0 changed pixels; the standing 115-era
numbers are 121's bandfill deltas, 121's log); the whole-game sweep
builds all 176 maps except the six elevations that already fail on the
123 base (124's log has the stash proof). The atlas note: the pinned
page repacks sorted by height, so its images' rects move - but every
image there is fly-only by construction, and the unpinned pack keeps its
order, so the game's sprites keep their texels (the pixel regression is
the proof).

Pairs (runs/125-widen-back-views/): the Den's game-visible CAR4b solid
(tile 16648, seen 0 - exactly the new population) from behind. Turn 240
is the statement: before, the far flank shows the near view's front
smeared; after, the art's own rear drawing. Turn 0 is the control. The
cola and shelf targets were shot but their framings failed (the machine
sits behind a fence, the shelf reads too small at the fly distance) -
their raw shots stay in runs/_125_shots/, and the whole-screen pixel
counts of the pairs are dominated by critter animation between the two
passes; the regression's zero is the honest number. The semantic risk
the owner may want to judge: a shelf's opposite slot could be a variant
rather than a true rear (122's turning gate rejected shelves for that);
the coverage gate cannot tell a rear from a variant - the pairs would
show it, this shelf's pair did not frame it.
