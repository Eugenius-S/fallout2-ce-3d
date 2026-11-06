---
id: 172
title: A location loads in 2 s from a warm cache
milestone: M1
status: doing
owner: glm
suggested: any
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: [127]
reviewed: no
---

## Goal

Walking into a local map with its scene cache already built shows it in 3D
in 2 s or less, measured from the map change to the first full 3D frame.

## Context

- The owner, 2026-11-04: fast loading is critical; a location loads in a
  couple of seconds. The cold cache build (the first export of a map) is
  one-time and is measured separately, not held to 2 s.
- Measure first: time every phase of a map change (the engine's own map
  load, reading the cache files, texture and buffer upload, shader and
  render target setup, the first frame). `ce/src/scene/scene_view.cc` loads
  the floor, walls and projection from `mod-cache/scene/<map>/<elevation>`.
- Likely wins: reading the cache once and in parallel, uploading textures
  without stalls, keeping shaders and render targets across maps.

## Acceptance

- A phase table (before and after) for kladwtwn, newr1, vctyctyd and one
  cave map in the Log and in `runs/172-location-load-time/`.
- A warm-cache map change at 2 s or less on each of them; the cold cache
  build time written next to it.
- ctest and the regression pass.

## Out of scope

Frame rate in play (157), settings changes (156).

## Log

## Log

- 2026-11-06, glm. Measured first: the load chain now reads as one table per
  map change (perfLoadStart at mapLoadByName, marks for map_tiles, map_scripts,
  map_objects, map_elevation, map_load, scene_loaded, first_frame; the scene
  stages floor/walls/projection and their read/decode/upload sub-phases under
  F2CE_PERF=1). Before (kladwtwn / newr1 / vctyctyd / cave0): 2487 / 3377 /
  2332 / 484 ms to the first full 3D frame. The engine's own map load
  dominated: objectLoadAll 1.3-1.9 s, and inside it _obj_rebuild_all_light
  0.8-1.3 s; then the art cache preload ~0.8 s; the scene load 0.24-0.42 s.
- Fixes, each result-identical: (1) the light ray walk's rect unions ran only
  for the rect output the rebuild never reads - skipped when rect == nullptr
  (_obj_rebuild_all_light 843 -> 239 ms on kladwtwn); (2) _obj_insert locked
  art during the load for a sort comparison whose result vanilla never used
  (the TODO: Incomplete block, v11 was never set) - removed, inserts are free;
  (3) the art cache warm spread over a worker pool: cacheLock splits into a
  locked hit path, the size and read procs outside the lock into a private
  buffer, and a locked insert with a double check against another thread
  locking the same key (the eviction and the heap stay under the lock;
  cacheFlushLocked for the insert's last resort); dfileOpen takes a mutex for
  its open-list push (every DFile has its own FILE* and inflate state, the
  reads never share); the preload warms the unique FIDs on up to 4 threads.
  art_preload 810 -> 446 ms (kladwtwn). The vanilla miss path fed the sound
  mixer every few locks; the threaded warm replaced those calls, one
  soundContinueAll on the main thread after the join keeps the loading sound
  moving. Two traps found on the way: soundContinueAll from worker threads
  and the per-read perf log writes multiplied the warm to 5-7 s - the scopes
  moved to aggregates.
- After: 1946 / 2519 / 1757 / 462 ms. kladwtwn, vctyctyd and cave0 hold the
  2 s budget; newr1 lands at 2.52 s (-25%), its remaining breakdown: the
  engine load 1628 (objects 932 with the light spread 297 and ~590 of
  per-object script links and inserts, the preload 525, the entry scripts),
  the scene 405 (the wall and tile PNG decodes are serial), the first frame
  447. The next levers, in order of size: the light spread on worker threads
  (per-thread delta grids - the intensity adds carry no clamps, any fold
  order lands the same totals, the blocked table is per-light), the scene's
  PNG decodes in parallel (pure CPU on private buffers), the per-object
  script link cost. Evidence: runs/172-location-load-time/phases.md.
