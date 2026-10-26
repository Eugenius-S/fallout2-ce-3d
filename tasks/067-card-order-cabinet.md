---
id: 067
title: A person standing before a cabinet draws in front of it
milestone: M1
status: done
owner: claude
suggested: glm
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A character standing in front of a tall thing (a cabinet, a locker) draws in
front of it, as in the game; the 3D view must not put the thing over the
person.

## Context

- The owner's live review of 2026-10-24, shots 25..35 in
  `runs/046-characters-in-shadow/review-2026-10-24/`: a cabinet draws in
  front of a person standing before it.
- Cards sort by `spriteRowPlane` (live_objects.cc); 051 just moved hanging
  smoke one row south. Find whether the cabinet's card or its fitted solid
  takes the wrong plane, then fix the rule, not the one cabinet.

## Acceptance

- Before and after snapshots of the case under
  `runs/067-card-order-cabinet/`, in the game's view and a turned one.
- The regression stays where it is.

## Out of scope

Shapes of the things themselves (068).

## Log

### 2026-10-26 claude
Done: the cause was the depth of solids in the game's view. The exporter
(`solids.py` `depth_home`) moves each depth point along the game's parallel
view ray to its card's row plane; under the game view's mild perspective that
point lands on another pixel, and the error there is far larger than the
5e-4 gap between two cards of one hex row. So the shelf's solid
(klamall, the hero at 13086) came out nearer than the hero standing before it.
Now a solid's face carries the plane its depth points span (`depthPlaneNormal`,
`sprite_card.cc`; vertex attribute 5 in `walls.cc`), and the fragment shader
(`scene_shader.cc` `planeDepth`) meets that plane with the pixel's own line of
sight and writes that depth. Faces of one solid share the plane bitwise, so
they tie exactly (no hatching). Live cards and the fly camera are unchanged
(zero normal; `uDepthAt` is 0 there). The rest cards on a solid's axis (a
pot's mouth, a table's stem) take their solid's board plane with its stack,
`2 * OVER_CARD` nearer (`wall_sidecar.py`, `rest_card` in `solids.py`), so
the solid no longer hides them.

Evidence in `runs/067-card-order-cabinet/`: `front.jpg` (hero before the
shelf: main hides him, now he draws in front, as in the original game),
`behind.jpg` (hero behind it stays hidden in both), `regression.jpg` (two of
the retaken references, before | after | original). Under a camera turned 45
degrees the hero before the shelf draws in front too.

Regression: the 15 changed spots are this fix (a solid's edge against its
neighbours, a rest card now over its solid); the references are retaken.
Against the original game's shots of the same views more changed pixels match
the after frame than the old reference on most spots (map-edge 1766 to 1026,
klamall 243 to 46, residents 386 to 210); central-doors and klagraz go the
other way where a squat pot model now shows its mouth (shapes are 068).

Next: none in 067.

Check: CTest 15/15, exporter tests 64/64. Regression: the 15 changed 3D-view
references retaken one by one (`-Spot ... -Update`): residents,
central-doors, north-rooms, map-edge, klagraz-e0, klamall-e0, klaratcv-e0..e2,
klatoxcv-e0..e2, klatrap-e0, and entrance and corrals (one pixel each at a
solid's edge; 0 on main); klacanyn-e0 kept (0 changed). A full run after
that: 16/16 with 0 changed pixels. The game's own 2D view is untouched: `-Classic` shots of the
case on main and on this branch differ by 0 pixels. Fly tour against main:
0 of 43 spots changed (`changed.jpg`).
