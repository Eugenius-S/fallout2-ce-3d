---
id: 114
title: The thing in the bathhouse wall: find what it is and put it where the game has it
milestone: M1
status: done
owner: glm
suggested: glm
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: no # the regression references were retaken
---

## Goal

The grey thing sticking out of the bathhouse's alley-side wall is gone from
the wall in every camera, and whatever it is stands where the original game
puts it.

## Context

- 100 (merged, #147) decided the thing was two `Junk` scenery piles in the
  alley (hexes 16478, 16479) and turned the mapper's loose clutter back
  into cards. The owner, 2026-10-28, live on main `ff5f4ed0` (with 100):
  the thing is still there, and it is not alley junk.
  - `runs/inbox/2026-10-28-bathhouse-wall-thing-iso.png`: the 3D view at
    the game's angle, the grey thing on the wall left of the hero, by the
    corner near the door (also `2026-10-28-bathhouse-corner-piece.png`).
  - `runs/inbox/2026-10-28-bathhouse-wall-thing-top.webp`: flight, roof
    off. Outside the alley wall lies a grey pipe; inside against the same
    wall stand a box, a plate and a can; the table with its things is on
    the right. The owner: the things on the table moved off it and stuck
    in the wall, physically moved.
- So 100 named the wrong objects. Start from the objects, not a theory:
  list every object (pid, hex, elevation, FRM, flags, the art's own x/y
  offset) whose picture lands on that stretch of wall, inside and outside,
  and the table's things; compare where each stands in the original view
  at the same scroll and in the 3D scene.
- Suspects, none proven: the frame offset of the art (FRM shift) taken or
  dropped twice; a lift (things standing on tables) that resolves to the
  wrong table or none; a card anchored at the wrong hex.

## Acceptance

- The Log names each object and the line that put it in the wall.
- before | after with the original at the same scroll, the 3D at the
  game's angle and in flight with the roof off, inside and outside, in
  `runs/114-bathhouse-thing-in-wall/`; the other tables of Klamath with
  things on them checked the same way.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

The shapes of the things themselves (115, 116).

## Log

### 2026-10-29 glm
Done: the thing in the wall is named and put back where the game has it.
It is object #1985, item 'Stuff' (pid 0x000000B2, art tbltop11.frm, the
pipe, box, plate and can of a laid table) at tile 18282 (82, 91) elevation
0 of kladwtwn: the clutter of Table ltable4 (obj 1984) by the bathhouse's
east wall. The line that put it into the wall: the lift (scene.py `lift`,
through scene.json "lifts" into baked.bin's LIFT) raises its card 0.55
units up the view ray; at the game's angle the raised rows crossed the
wall's silhouette and the lattice's open upper rows, and read as a grey
pipe growing out of the alley wall (proven by flipping the card's baked
entry to Skip: exactly the pipe's 693 px vanish). The lift's own "same
pixels" contract does not hold in the renderer: the view's perspective
moves a lifted card tens of pixels up-screen.
Fix, two rules in the renderer (ce/src/scene_renderer):
- a lift raises a card only under the fly camera; the game's camera keeps
  the game's own pixels, and the wall hides the card as the game hides it
  (live_objects.cc);
- an indoor card is cut at the roof plane where a roof covers it
  (uRoofClip + the roof grid: scene_shader.cc, roofs.cc, scene_draw.cc):
  the game's roof picture hides a room's cards above that plane, the 3D
  roof sat lower and left the band open.
The regression references encoded the lifted cards, so they were retaken
with `-Update` on this commit (16 spots, all self-consistent; klamall,
klagraz-e0, central-doors, map-edge, klaratcv-e2, klatoxcv-e0, klatrap-e0
PNGs changed): per the rules this PR is `reviewed: no`.
Next: the tour's changed.jpg attached here; the owner checks the bathhouse
(user-check).
Check: runs/114-bathhouse-thing-in-wall/ (before | after at the game's
angle, the classic original, flight with the roof off, the klamall tables'
before | after); ctest 18/18; the scene_export and regression unit tests.
