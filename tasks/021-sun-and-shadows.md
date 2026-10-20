---
id: 021
title: Sun by game time and building shadows
milestone: M1
status: todo
owner: none
suggested: claude
claude-only: no
user-check: yes
depends-on: [013, 020, 034]
reviewed: no
---

## Goal

A sun that moves across the sky with game time and casts sharp shadows from
buildings and big things; characters darken in shadow.

## Context

- Game time comes from the engine; it jumps on waiting, travel and save load.
- Walls, roofs and furniture solids are geometry; character sprites cast no
  shadows.
- Casters, by the owner (2026-10-20, `docs/2026-10-20-shapes-and-rotation-design.md`):
  walls with their backs and ends, roofs, doors, and big solids (at least two
  hexes across or taller than half a wall). Small things cast no sun shadow
  and keep the shadows painted into their art. This narrows the Plan below.
- What a hidden roof does to shadows is decided in task 013.

## Plan (claude, 2026-10-19)

- Sun position from `gameTimeGetTime()` (36000 ticks an hour, `scripts.h`):
  height by hour; strength and color follow the ambient the map scripts set,
  so the game's own day and night decide how bright the sun is. The azimuth is
  tuned on snapshots with the owner.
- No sun where `scene.json` says `indoor` (underground); `daylight` marks maps
  under the sky.
- One directional shadow map (depth texture, OpenGL 4.1) over the visible
  ground plus a margin. Casters: walls, trims, doors, solids, forms, roofs
  (hidden ones too, see 013). Receivers: floor, walls, and sprites by one
  lookup at the anchor slightly above ground, smoothed over ~150 ms.
- The game art has shadows painted in. The exporter already gives the
  shadow-free sprite of each such board (`shadows`: board, sprite without the
  painted shadow, shadow sprite) and floor shade (`casts`). With the sun on,
  use the shadow-free sprites and drop `casts`; with it off, the originals.
- Pixelation: snap shadow lookups to a world grid of N game pixels.
- A jump of game time (wait, travel, load) sets the sun at once instead of
  sweeping it.

## Acceptance

- Sun direction, height and color follow game time: long shadows morning and
  evening, no sun at night, no sun inside interiors. Time jumps do not flicker.
- Sharp shadows that follow open doors, hidden roofs and changed objects;
  an adjustable slight pixelation of shadows in the original graphic style,
  stable while the camera and the sun move.
- A sprite whose anchor is in a building's shadow darkens as a whole, keeping
  ambient light and other lamps, with a short transition at the border.
- `[3d] sun`, `[3d] shadows`, `[3d] shadow_pixel_size` with menu labels.
- Snapshots at noon, sunset and night; a character inside and outside a
  shadow; the frame time with shadows on and off in the `Log`.

## Out of scope

Cloud shadows (023).

## Log

### 2026-10-19 claude
Done: research; findings and plan in `## Plan`.
Next: start when the dependencies are done.
Check: read `## Plan`.
