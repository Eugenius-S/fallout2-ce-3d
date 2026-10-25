---
id: 080
title: Roofs and overlapping walls in 3D: nothing sticks out, nothing shows through
milestone: M1
status: todo
owner: none
suggested: claude
priority: normal
claude-only: no
user-check: yes
depends-on: [076]
reviewed: no
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
  `runs/080-roofs-and-overlaps/`.
- The game's own view unchanged.

## Out of scope

Things and furniture (075).

## Log
