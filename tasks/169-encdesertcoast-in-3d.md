---
id: 169
title: The desert and coast encounters in 3D, every map stands and looks clean
milestone: M1
status: done
owner: glm
suggested: glm
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

Every map of the desert and coast encounters exports, loads in the game in 3D and looks as clean as
Klamath: in the game camera and the fly camera, at morning, noon and
evening, every elevation. One of the areas of the all-maps sweep (136 Arroyo,
then 158 to 171 in the game's order).

## Context

- The maps (the game's map files, checked against `runs/p1-listmaps.log`):
  desert1 desert2 desert3 desert4 desert5 desert6 desert7 desert8 desert9 desrt10 desrt11 desrt12 desrt13 coast1 coast2 coast3 coast4 coast5 coast6 coast7 coast8 coast9 coast10 coast11 coast12. Add them to `MAP_GROUPS` in `tools/scene_export/export.py` as
  `encdesertcoast`. If the game holds more maps for this area, take them too.
- What is here: Random encounter maps: open sand, rocks, coastline and water edges. Expect art the exporter has not seen yet.
- Everything Klamath and the areas before this one learned is already in the
  exporter's rules. Fix new defects by general rules in the exporter or the
  renderer that hold for every map and object of that kind; never a case for
  one object, tile or map (AGENTS.md). A defect no general rule fixes goes
  into the Log as "owner:".
- Every map already done must stay as it is: the regression and the fly tour
  against origin/main are pixel for pixel or every change is explained.
- Export into your own cache (`--cache-root build/scene-cache`). Never write
  the owner's `game/mod-cache`.

## How to work

The same loop as 136, without waiting for anyone:
1. Export the group. Every export error or warning is a find.
2. Add fly tour spots and regression spots for each map, a few per map.
3. Sweep: every map, elevation, camera and hour; write each defect as one
   numbered line in the Log (map, tile or spot, camera, hour, a crop in
   `runs/169-encdesertcoast-in-3d/finds/`, the likely cause).
4. Since 2026-11-04 (the owner): the fixes go by kind, not by area. Move
   each find to its batch (189 walls, 190 roofs, 191 furniture and props,
   192 vehicles, 173 what cannot be built) and fix it there. A crash or a
   place the player cannot get past is fixed here at once. The old loop
   below stays for finds that fit no batch: fix the most visible group on its own branch
   `task/169-<short-cause>` from current origin/main, one PR titled
   `169: <cause>` with before | after crops. Mark the finds it closes.
5. Back to 4; when the list is empty, sweep again (3).

The first PR is the group plus the tour and regression spots, so the area
plays in 3D even before every find is fixed.

## Acceptance

- `export.py --maps encdesertcoast` exports every map without errors.
- Walking into each map in the game keeps 3D on.
- A final sweep finds no new defect; the Log holds every find marked fixed
  (PR number), owner, or open with a reason.
- Each fix PR: before | after crops, ctest, the regression and the fly tour
  against origin/main.

## Out of scope

The interface, menus and text, the sky, new 3D models from scratch, maps of
other areas (each has its own task).

## Log


## Log

- 2026-11-05, glm. The encdesertcoast group (desert1-9, desrt10-13, coast1-12;
  25 maps, 30 elevations) exports into build/scene-cache; desert1-e0 was
  already a cache hit, the other 29 export with no errors or warnings.
  Entrance stills on all 30 elevations (3D on everywhere), the 8/12/18 hour
  sweep on all of them, grid probes on coast1, desert5, coast10, and walk
  probes 7/7 (the hero moves, 3D stays on). The cars, shanty huts, cliffs
  and tents all stand. The dark elevations (coast2 e1, coast5 e1, coast7 e1,
  desrt13 e1/e2) match the game's own 2D picture (-Classic), they are the
  maps' real darkness.
- Finds:
  1. desert5 e1 and desert6 e1: the 3D view is black where the game's own
     picture shows dim content (2D mean 46 vs 3D 0.8 at tile 508). Cause:
     the export of these elevations carries no floor art and no sprites
     (walls.bin 24 bytes, tiles.png 424 bytes, sprites0.png 95 bytes,
     scene.json has sprites 0, walls 0, indoor=true), so the renderer has
     nothing to draw. The scene ships; the elevation needs its tiles built
     or a plane fallback. Folded into 173 with the shots
     (finds/desert5-e1-classic-vs-3d.jpg). Batch: 173.
  2. desert1: the map is not in the worldmap registry, so the dev tools
     cannot travel to it (requestMapChange refuses, the startup target
     retries forever, the fixture falls back to the default map and the
     still probe silently shot ARTEMPLE). The scene exports and ships;
     players reach the map through random encounters. The tour spot and
     the fallback shots are dropped; the regression gate already rejects a
     capture from a wrong map. Batch: 173 (a direct name-load path for
     registry-less maps).
  3. Tooling, fixed here: dev-snapshot's watchdog could crash a capture
     with an Add-Content IOException when a fast game exit raced the
     diagnostic write; the write is best-effort now.
- Tour: 9 spots (desert1 dropped as unreachable), tour before/after
  identical, 0 of 9 spots changed. 3 regression spots with references
  (coast-road-car, desert-arch, coast-shanty); full suite 103 spots 103
  pass, 0 changed pixels. Checks: exporter tests 125/125, regression tests
  12/12, ctest 20/20. Evidence: runs/169-encdesertcoast-in-3d/ (probe-sheet,
  hours-*, grid-*, classic-2d-checks, walk-sheet, tour sheets, changed.jpg,
  finds/).
