---
id: 045
title: Buildings and big things cast sun shadows
milestone: M1
status: todo
owner: none
suggested: gpt
claude-only: no
user-check: yes
depends-on: [021]
reviewed: no
---

## Goal

Sharp sun shadows from buildings and big things fall on the floor and on
walls, and follow open doors, hidden roofs and changed objects.

## Context

- The sun comes from 021.
- Casters, by the owner (`docs/2026-10-20-shapes-and-rotation-design.md`,
  "Shadows"): walls with their backs and ends, roofs (hidden ones too, see
  013), doors, and big solids (at least two hexes across or taller than half a
  wall). Small things cast nothing and keep the shadows painted into their art.
- One directional shadow map (a depth texture, OpenGL 4.1) over the visible
  ground plus a margin. Receivers: the floor and walls.
- The original style: snap shadow lookups to a world grid of N game pixels
  (`[3d] shadow_pixel_size`), stable while the camera and the sun move.

## Acceptance

- `[3d] shadows` and `[3d] shadow_pixel_size` with menu labels; with shadows
  off the game view is exactly as before (the regression passes).
- Snapshots in `runs/045-building-shadows/`: noon, sunset and night at the
  regression spots, and an open door.
- The frame time with shadows on and off in the `Log`.

## Out of scope

Characters in shadow and the painted shadows (046), clouds (023).

## Log
