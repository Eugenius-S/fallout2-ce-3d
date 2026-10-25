---
id: 076
title: Walls, corners and roofs in 3D: one rule instead of patches
milestone: M1
status: todo
owner: none
suggested: claude
claude-only: no
user-check: yes
depends-on: [071]
reviewed: no
---

## Goal

From any fly camera angle buildings read as whole: walls meet cleanly at
every corner, no wall shows through another, nothing sticks out of a roof.
The fix is in the rule that builds walls and roofs from the map, not in
patches for single spots.

## Context

- The owner's second live review after 071 round 4 (2026-10-25,
  `runs/inbox/2026-10-25-review2-01..10.png`, the list in
  `tasks/inbox.md`); their verdict: fix the 3D-fication algorithm itself,
  no hand-made cases.
  - 01: the STOP corner still meets wrong.
  - 02, 05, 06, 07: dark buildings' corners show a striped strip of light
    or other material down the corner.
  - 03: a wall shows through another wall.
  - 08: walls and triangular pieces stick out of the roofs.
  - 10: a house corner broken next to a door.
- Where 071 left it: corners are closed by matching wall ends in the
  exporter (`tools/scene_export/wall_sidecar.py`, `_corner_fits`), fly
  camera only; the corner picture is borrowed from the neighbouring wall.
  The stripes likely come from that borrowed picture (the donor's corner
  sprite belongs to one wall's line). Roof squares lie on the map grid and
  need not match the wall lines, which may be what sticks out.
- Directions to weigh: build each building's outline once (walls as a
  closed polyline per elevation) and derive corners, caps and roof
  edges from it; take corner faces from the corner sprite itself; clip
  roofs to the outline and walls to the roof.
- The game's own view stays pixel for pixel (`dev-snapshot.ps1 -Map
  kladwtwn|klamall` against the pre-071 shots: 0 px).
- The owner may add more from the same review.
- This task takes over 060 (Vic's shack from the back: roof gap, open
  corner, hanging sign); close it with it.

## Acceptance

- The owner's spots read clean from turned views, before | after in
  `runs/076-walls-roofs-algorithm/`.
- The game's own view unchanged (regression 16/16).

## Out of scope

Things and furniture (075).

## Log
