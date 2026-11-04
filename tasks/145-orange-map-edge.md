---
id: 145
title: No orange dithered band at the Klamath map edge in 3D
milestone: M1
status: todo
owner: none
suggested: gpt
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The 3D view shows nothing at the map's edge that the game's 2D picture does
not: no orange, dithered, see-through band over the ground before the black.

## Context

- Found by Claude, 2026-11-01, on main `b056099a` in the background test
  run: `tools/dev-snapshot.ps1 -Map kladwtwn -Still -Hour 8 -ViewTile 24678`
  (and `-Zoom 0.5`). In the upper right, next to the black beyond the map, a
  wide band of ground is tinted orange with a fine dither pattern and ends in
  a slanted straight line: `runs/inbox/2026-11-01-orange-map-edge.jpg`.
- The snapshot JSON shows `map_clip` enabled, `source: authored`. Suspects:
  the map clip's edge fade, a scroll blocker or exit grid drawn as a tinted
  plane, a tile outside the authored bounds. Find which first.

## Acceptance

- The cause in the Log.
- before | after of the same view in `runs/145-orange-map-edge/`, plus the
  game's own 2D picture of it (`-Classic`); the fix is a general rule, not
  a special case for this map.
- ctest passes; the scene regression changes only at the map edge.

## Log

- 2026-11-01 Cut from Claude's 137 captures.
