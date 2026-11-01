---
id: 124
title: The carve's far sides take the art's own back views
milestone: M1
status: done
owner: glm
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: [123]
reviewed: no
---

## Goal

The 120 carve's far sides show the near view's pixels mirrored. The arts
hold their own opposite views (the game's six stored directions; truck2 and
truck3's opposite slots are byte-equal mirrors of the near views, so the
mirror was the artists' own convention) - where a real opposite view exists,
the fly camera should show it, not the mirrored near view.

## Context

- solids.solid_faces textures the away faces from the near faces' points
  (the mirror). The machinery is generic; this slice gives the carve rows a
  second sprite, the art's own back view, and points the away faces at it.
- The opposite direction is (rotation + directions / 2) % directions for
  arts with 2 or 6 stored directions; 1 and 3 have none.

## Acceptance

- Fly pairs of the Den's vehicles from behind: the far sides show the arts'
  own back drawings, no see-through texels anywhere.
- The game's views stay pixel for pixel on the regression spots (away faces
  are culled by the game's camera); ctest passes; the whole-game sweep builds.

## Out of scope

Non-carve rows (their away faces keep the mirror), drums, the engine.

## Log

### 2026-11-01 claude review
Done: merged task 123 and current main; exporter tests 88 OK. The runs-size check failed on four PNG pairs over 1 MB: converted the five big pairs in runs/124-carve-back-views/ to JPEG under 0.8 MB each. The game-view regression and the fly tour were run once on the combined 124+125 tip (see task 125's log), since both only add away-face sprites that the game's camera culls. The pairs show the changed far sides (turn 120: 40736 changed px; the turn 180 control: 0).
Check: runs/124-carve-back-views/*.jpg.

### 2026-11-01 glm: the recon: opposite slots measured, the composite closes the holes

The Den's scenery arts through the carve's own qualification
(real directions, long-low profile), the opposite slot (rotation + 3) of
every real rotation, the share of the near body's opaque texels that the
opposite body covers at the mirrored projection (runs/_124_recon/):

- CAR4b 0.68 (all three pairs), CAR5 0.74, TRUCK3 0.55 / 0.78,
  TRUCK2 0.82 one way and 0.38 the other (its back view is much smaller),
  CAR1 0.10 one way (its opposite slot holds a tiny separate drawing).
- CAR3b, CAR3, CAR4, TIRE004, SS122: every opposite slot is the placeholder
  card or repeats the near slot.

The design that follows: a carve row's back sprite is composed in the near
view's own pixel frame - the opposite body where it is opaque (aligned by
world position), the near body's own texel where it is not - so the back
face never samples a transparent texel whatever the two silhouettes do. The
gate is the coverage share (BACK_COVER = 0.5: the real pairs pass, CAR1's
tiny slot and TRUCK2's much smaller back stay on the mirror). The away faces
then sample that sprite with the mirrored mapping; a byte-mirror pair reads
exactly as today. The sprite rides the row (row[17]) and pins to the atlas's
trailing page with the other fly-only images.

### 2026-11-01 glm: the back views landed on the carve rows

The compose and the two gates (the placeholder slot, the coverage share)
landed in scene.back_sprite; solids.solid_faces points the away faces at
row[17] with the mirrored mapping, keeping the plain mirror without the
field. The alignment lesson: the opposite frame's placement takes the
object's own x / y like the near one does - without them a placement with
pixel offsets failed the coverage gate for no reason. The row layout puts
the back view at 17, behind an empty 16 that reads as a box's no clip
planes (the drums keep 16 for their own planes, and a drum is never a carve).

On the Den the gates gave backs to CAR4b's rot 3 and rot 4 placements
(cover 0.66-0.69); CAR3b (placeholder slots), car3 rot 0 (cover 0.25) and
the smaller opposite slots keep the mirror, car3 rot 3 and CAR5's placements
would pass wherever they carve. The pairs in runs/124-carve-back-views/:
the two Den cars from behind at the three back quarters plus the two
controls (the front quarter and the quarter that sees no far side at all) -
before: the near view's mirror smear; after: the art's own rear drawing,
no see-through texels anywhere.

Checks: the exporter unittests 87 (five new: the compose's probes and
alpha-superset over the near body, the placeholder gate, the coverage gate,
the no-opposite-directions gate, the wall-sidecar row-to-faces mapping);
ctest 18/18 (the engine untouched); the 16 regression spots on the new code
are PIXEL-IDENTICAL to the 123-rebar run's captures (0 changed pixels
everywhere; the numbers against the 115-era references are the standing
121-bandfill deltas, 121's log); denbus1's walls.bin game batches are
byte-identical to the 123 cache's and the moved atlas rects are exactly the
carve rows' own fly-pinned near sprites (the unpinned pack provably keeps
its order: the back images join the pinned set only). The whole-game sweep
builds every map through the new exporter except the six elevations that
already fail on the 123 base (coast11 e0, encfite e1 / e2, epamain1 / 2 e2,
epax e1) - proven by re-exporting those maps on the stashed base code and
seeing the same four messages.
