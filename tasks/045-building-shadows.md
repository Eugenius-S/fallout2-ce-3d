---
id: 045
title: Buildings and big things cast sun shadows
milestone: M1
status: doing
owner: glm
suggested: gpt (bailed 2026-10-23, the owner passed it to glm)
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
- A quick time of day, asked by the owner: a dev key in the panel steps the
  game hour (for example by 3 hours) and `--dev-hour=H` sets it on start, so
  morning, noon, sunset and night can be looked at in a minute; the sun is
  set at once, as on a time jump (021).

## Out of scope

Characters in shadow and the painted shadows (046), clouds (023).

## Log
