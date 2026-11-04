---
id: 147
title: Map exits, elevations and the world map come back in 3D
milestone: M1
status: todo
owner: any
suggested: any
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Every way the hero leaves a Klamath map, changes elevation or returns from
the world map ends on the new map in 3D: the right scene, no stale scene
from the old map, no long 2D frame, the camera on the hero.

## Context

- 010 gives a scene for any map and elevation; nobody has walked the
  transitions themselves since the 3D view stays on more often (135).
- Ways to cover: each exit grid between the Klamath maps (kladwtwn,
  klamall, klagraz, klacanyn, klaratcv, klatoxcv, klatrap), stairs and
  ladders between elevations (klaratcv, klatoxcv), leaving to the world map
  and entering a map from it, loading a save made on another map.
- The background run can travel (`-Tour`) and load maps; add what is
  missing to drive an exit grid or an elevation change, or explain in the
  Log why a step was checked by hand.

## Acceptance

- A table in the Log: each transition, what the first 3D frame on arrival
  shows (crop in `runs/147-map-transitions-keep-3d/`), and whether a 2D
  frame or the old scene flashed.
- Every defect found fixed by a rule, or cut as its own task with its crop.
- ctest passes; the regression stays pixel for pixel.

## Log

- 2026-11-01 Cut with the 146 pool.
