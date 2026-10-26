---
id: 081
title: Roofs and overlapping walls in 3D: nothing sticks out, nothing shows through
milestone: M1
status: done
owner: claude
suggested: claude
priority: normal
claude-only: no
user-check: yes
depends-on: [076]
reviewed: yes
---

## Goal

From any fly camera angle within 45 degrees no wall piece or triangle
sticks out of a roof, no wall shows through another, and a house corner
next to a door reads whole.

## Context

- What is left of the owner's second review after 076 closed the
  corners (`runs/inbox/2026-10-25-review2-*.png`):
  - 03: a wall shows through another wall (a light patch in a dark wall);
  - 08: walls and triangular pieces stick out of the roofs;
  - 10: a house corner broken next to a door (an open door leaf, hidden
    since 072, and the corner's material).
- Roof squares lie on the map grid and need not match the wall lines;
  the walls' rise faces and the donor's trims at a roof edge are the first
  suspects for 08.
- `tools/scene_export/corner_rays.py` is the pattern for a script check.
- The game's own view stays pixel for pixel.

## Acceptance

- The owner's spots read clean from turned views, before | after in
  `runs/081-roofs-and-overlaps/`.
- The game's own view unchanged.

## Out of scope

Things and furniture (075).

## Log
### 2026-10-26 claude
Done: the owner's spots read clean for the free camera; the game's view is
untouched.
- Floating corner (inbox 2026-10-25, owner's
  `corner-wall-floats.png`): main showed it too. Many wall pictures end a
  little above the floor, so in flight the bricks hung over the street and
  the corner core stood alone. Walls, corner fills and end faces that end
  at most `FOOT_MAX` (0.5) above the floor get a foot down to it for the
  free camera, in the wall's lowest colours. The see-through bricks of the
  owner's shot were not seen on main.
- 08, roofs: a wall's face rose past the roof over its square (the roof
  squares need not match the wall lines). The free camera now cuts wall
  faces at the roof height over roof squares (`_cut_by_roofs`); the game
  keeps its triangles whole in the game-only batch. Points above a roof on
  kladwtwn: 1164 -> 0. The engine draws a roof cell's rim 6/88 past its
  square; a wall there is cut only where the rim's texel is opaque (most
  Klamath rims are clear, and the game shows the wall through them: cutting
  the whole rim took the top off a klamall mesh fence).
- 10, the corner by a door: a corner picture (a post) ran on past the
  crossing out of the corner reach, so the corner pass saw no corner and
  the piece stood out into the street as a fin. A last row that crosses the
  other wall's band and stops within twice the reach past it now ends the
  slab in the crossing (`tip` in `_corner_fits`); the piece folds like any
  corner piece. Only this corner changed on the Klamath maps.
- 03, a wall through a wall: not seen on main. Every dark facade of
  kladwtwn shot frontally, and script checks for rows poking through
  another wall's front and for gaps in a front found nothing but a crate
  (075). Likely closed by 076/083.
- `walls.bin` format unchanged (version 5); only its content changed.
- The game's own view: the 16 regression spots of this branch's cache and
  of main's match 0 px. The regression references fail the same way on
  main's cache (hydrants, klatoxcv-e1 22777 px), so that drift is on main.
- corner_rays: kladwtwn 3 corners at 2 rays, as before; klagraz, klatrap,
  klacanyn 0. Exporter tests 64/64.
- Before | after: `runs/081-roofs-and-overlaps/corner-floats-before-after.jpg`,
  `roofs-before-after.jpg`, `corner-by-door-before-after.jpg`.
Next: the regression references need a look against main.
Check: `py -3 -m unittest discover -s tools/scene_export -p 'test*.py'`.
