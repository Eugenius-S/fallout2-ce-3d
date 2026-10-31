---
id: 123
title: Burnt rebar stands: second-floor wall ruins continue the wall
milestone: M1
status: doing
owner: glm
suggested: owner
priority: normal
claude-only: no
user-check: yes
depends-on: [122]
reviewed: no
---

## Goal

The owner, 2026-10-31: on some buildings' second floors the burnt rebar
(the scorched reinforcement sticking out of the ruin) renders lying down;
it must stand as a continuation of its wall, not lie on the floor or the
roof.

## Context

- The rebar art rides the wall ruins (second-floor roof edge pieces); today
  it lands as a card that lies along the surface instead of standing on the
  wall's plane.
- The fix is exporter-side: such pieces belong to their wall's upright plane
  (a panel or sheet on the wall), like the wall scenery rule of 100/116
  (wall scenery keeps wall-sheet sorting).

## Acceptance

- Before | after pairs of the affected buildings: the rebar stands on the
  wall line, no piece lies flat across a roof or floor.
- The game's views stay pixel for pixel on the regression spots; ctest
  passes.

## Out of scope

New art, engine work, other ruin fixes.

## Log

### 2026-10-31 glm: the recon: three wdbeam populations, the carve is the liar

The rebar arts are the WDBEAM files (wdbeam01-10, 20-25): a leaning burnt
frame (a post and a beam), two real views plus four UNUSED ART placeholder
cards each. 656 placements in three populations:

- 375 WALL-type objects: walls.classify takes them by their proto's ext flag
  (0x2000, source=flag), their faces stand on the wall line - these already
  stand.
- 276 SCENERY objects (+5 the game itself flags FLAT - decals, they lie by
  the game's own choice): image_axis says nothing about the leaning art, so
  things.stand makes a card; the plain fit refuses, and 120's carve fallback
  picks them up - real_directions >= 2, the forced box covers 0.7 - and
  raises ground boxes to the art's height bands. From the fly quarter the
  box bands read as BEAMS LYING ON THE GROUND (the abbey shots,
  runs/_123_recon/wdbeam9303-state.png): the owner's complaint, reproduced
  on the current stack. The carve is the right machine for the long-low
  car profile and the wrong one for a tall leaning frame.

The plan, both halves algorithmic, no name lists:

1. The carve gate gains the long-low condition: the carve stands for the
   game's big multi-quarter things - a car - and only an art whose real
   views are long and low qualifies (measured: the vehicles sit at
   width/height 1.39-2.57, the wdbeams at 0.13-1.02).
2. A scenery piece the carve declines joins its wall: where a wall run
   passes through the hexes beside it (join_odd_walls' own test), the piece
   gets the doors' PLANE treatment - the live card stays the game's pixel
   for pixel, and the fly camera draws it standing on the wall's plane
   (live_objects.onFlyPlane; the doors prove the machinery). Abbey's
   wdbeam tiles: 8 of 9 have the run beside them; the ninth keeps a plain
   standing card, which still beats a lying box.

Branch note: task/123-rebar-stands off task/122 (the stack's tail).

### 2026-10-31 glm: the carve is long-low only; the rebar stands on its wall run

Both halves landed on the exporter alone, no engine changes:

- props.real_aspect + the CARVE_LONG = 1.35 gate: the carve qualifies only
  arts whose real views are long and low (the vehicles measured 1.39-2.57;
  the wdbeams 0.13-1.02; ss122 at 1.36 stays, crafter2 at 1.28 leaves -
  its "carved" rows game-wide turned out to be the 119 lid machinery, not
  the carve, and are untouched). The Den's cars keep their carve: the
  cache-123b re-export stands the car4b exactly as 120's did (the texture
  difference is 121's fill, which sits above in the stack).
- scene.wall_join: a tall qualified piece the carve declines joins the wall
  run that passes beside its hex (join_odd_walls' own test) - the exporter
  writes a wallCards row [ua, va, ub, vb, object] (the run's line through
  the piece's extent), and baked_sidecar turns it into baked.bin's PLANE
  entry, the doors' machinery: the game's views keep the live card pixel
  for pixel, the fly camera draws it standing on the wall's plane
  (live_objects.onFlyPlane). Abbey e0: 6 of the 9 scenery wdbeams joined
  (the other three keep their pre-existing treatments: one is walls-batched
  SKIP, two take accepted fits). The carve's props rows, rests and hidden
  marks for the joined pieces are GONE (abbey props 186 -> 163 rows, all
  removals the wdbeam carve's).

Checks: the exporter unittests 82 (two new: the long-low gate, the
wallCards row to PLANE); the whole-game build sweep clean (268 elevations
build, only the two pre-existing truncated epamain third elevations fail);
the 16 regression spots on the new code are PIXEL-IDENTICAL to the
121-bandfill run's captures (the standing references carry 121's fill
deltas, awaiting the owner's visual pass, 121's log). The pairs in
runs/123-rebar-stands/: the abbey 9303 state (game | fly), the join
before | after at four fly quarters plus the turn-90 zoom, the Den's car4b
unaffected, the gstcav1 turn-0 pair (its turn-180 quarter renders the
cave's void in both states). gstcav1's cave keeps the rebar subtle: dark
art on dark rock; the owner judges the pairs.
