---
id: 147
title: Map exits, elevations and the world map come back in 3D
milestone: M1
status: doing
owner: gpt
suggested: gpt
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
- 2026-11-02 Added opt-in `--dev-transition-watch` instrumentation to record
  present mode, map/elevation, iso state and hero tile across a map identity
  change. Added an opt-in exit-grid probe to the background snapshot runner.
  These are diagnostic tools; no render policy or game content was changed.
- Runtime evidence, background instance, KLAMALL exit tile 28078 to
  KLADWTWN/e0 arrival tile 25142:

  | Transition | First arrival frame and result | 2D/old-scene observation |
  | --- | --- | --- |
  | KLAMALL/e0 -> KLADWTWN/e0 | First captured 3D frame has KLADWTWN/e0, hero tile 25142; comparison: `runs/147-map-transitions-keep-3d/klamall-to-kladwtwn-first-frames.jpg` | The captured first arrival present is black loading background with HUD, followed by 3D. This alone does not establish whether any old scene flashed or whether the loading duration is acceptable. No defect conclusion. |
  | KLAGRAZ/e0 -> KLADWTWN/e0 | Not reached | Probe selected exit tile 22161, target tile 19850/e0, but projected click at (463,232) was not pickable; no transition or arrival capture. |
  | Other Klamath exits, elevation changes, world map out/in, save on another map and reload | Not checked | No runtime evidence. |

- In the measured KLAMALL -> KLADWTWN sequence, watcher recorded 167
  presents between map identities. The last 2D callback was at 23962 ms and
  the first 3D callback at 24092 ms, 130 ms apart. The 2D callback is marked
  `iso=1` / `available=iso-disabled` during map loading; do not treat the
  measured loading callbacks or this black frame as acceptance of the
  "no long 2D frame" requirement, and do not change loading fades from this
  evidence alone.
- Capture stop condition reached after two unsuccessful generic exit-probe
  attempts. The first aborted while overlay sync was off; after fixing that
  probe guard, the second failed because the projected KLAGRAZ exit was not
  pickable. Stop further runtime capture until the probe can use the normal
  movement/picking path or a verified pickable exit.
- `ctest --test-dir build --output-on-failure`: 18/18 passed. This does not
  validate transition coverage or the pixel regression requirement.
- Status remains `doing`. Next: fix and validate the exit probe, capture each
  listed route/elevation/world-map/save-load path with first-frame sequence,
  determine whether any 2D or old-scene flash is actually visible, then run
  the pixel regression and update this table. No transition defect is yet
  confirmed or fixed.
