---
id: 145
title: The 3D map edge matches the game, cut where it cuts, with no orange band
milestone: M1
status: todo
owner: any
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The map edge batch. The 3D view shows nothing at the map's edge that the
game's 2D picture does not: no orange, dithered, see-through band over the
ground before the black, and the edge is cut where the original cuts it.

## Context

- Batch: the map edge (2026-11-04 inbox sweep). Folded in:
  - the inbox line of 2026-10-21: the 3D scene cuts the map edge farther out
    than the game (kladwtwn, the bottom edge by the cornfield,
    `runs/inbox/2026-10-21-map-edge-cut.webp`, the red line is where the
    original cuts). First screenshot the exit zones of the original maps to
    learn where the game cuts, then fit the clean edges (016) to that;
  - the inbox line of 2026-11-03: an orange translucent wedge at the map
    edge in fly shots, the same band as below.
- Found by Claude, 2026-11-01, on main `b056099a` in the background test
  run: `tools/dev-snapshot.ps1 -Map kladwtwn -Still -Hour 8 -ViewTile 24678`
  (and `-Zoom 0.5`). In the upper right, next to the black beyond the map, a
  wide band of ground is tinted orange with a fine dither pattern and ends in
  a slanted straight line: `runs/inbox/2026-11-01-orange-map-edge.jpg`.
- The snapshot JSON shows `map_clip` enabled, `source: authored`. Suspects:
  the map clip's edge fade, a scroll blocker or exit grid drawn as a tinted
  plane, a tile outside the authored bounds. Find which first.

## Acceptance

- kladwtwn's bottom edge by the cornfield is cut where the original cuts it
  (a shot next to the original's).

- The cause in the Log.
- before | after of the same view in `runs/145-orange-map-edge/`, plus the
  game's own 2D picture of it (`-Classic`); the fix is a general rule, not
  a special case for this map.
- ctest passes; the scene regression changes only at the map edge.

## Log

- 2026-11-01 Cut from Claude's 137 captures.
