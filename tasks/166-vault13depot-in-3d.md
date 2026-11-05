---
id: 166
title: Vault 13 and the Sierra Army Depot in 3D, every map stands and looks clean
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

Every map of Vault 13 and the Sierra Army Depot exports, loads in the game in 3D and looks as clean as
Klamath: in the game camera and the fly camera, at morning, noon and
evening, every elevation. One of the areas of the all-maps sweep (136 Arroyo,
then 158 to 171 in the game's order).

## Context

- The maps (the game's map files, checked against `runs/p1-listmaps.log`):
  v13ent vault13 depolv1 depolva depolvb. Add them to `MAP_GROUPS` in `tools/scene_export/export.py` as
  `vault13depot`. If the game holds more maps for this area, take them too.
- What is here: A vault's corridors and the deathclaws' rooms; the depot's military bunker, fences and robots' halls. Expect art the exporter has not seen yet.
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
   `runs/166-vault13depot-in-3d/finds/`, the likely cause).
4. Since 2026-11-04 (the owner): the fixes go by kind, not by area. Move
   each find to its batch (189 walls, 190 roofs, 191 furniture and props,
   192 vehicles, 173 what cannot be built) and fix it there. A crash or a
   place the player cannot get past is fixed here at once. The old loop
   below stays for finds that fit no batch: fix the most visible group on its own branch
   `task/166-<short-cause>` from current origin/main, one PR titled
   `166: <cause>` with before | after crops. Mark the finds it closes.
5. Back to 4; when the list is empty, sweep again (3).

The first PR is the group plus the tour and regression spots, so the area
plays in 3D even before every find is fixed.

## Acceptance

- `export.py --maps vault13depot` exports every map without errors.
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

- 2026-11-04, glm. The vault13depot group (v13ent, vault13 e0-e2, depolv1
  e0-e1, depolva e0-e2, depolvb e0; 10 elevations) exports clean into
  build/scene-cache with no errors or warnings. Entrance stills, a grid
  probe on v13ent, vault13 e0/e2, depolv1 e0, depolva e0/e1, depolvb e0,
  and the 8/12/18 hour sweep on all 10 elevations show every room standing:
  the vault corridors and round doors, the deathclaw floor, the depot
  gatehouse with its fences, the robot storage halls and the robotics bay.
  Walk probes 6/6 keep 3D on (hero moves tile to tile; direct map travel
  probes). The dark views (depolv1 e1, depolva e2, v13ent e0) match the
  game's own 2D picture (-Classic), they are the maps' real darkness, not
  missing light. No confirmed scene defects; nothing to move to 189-192 or
  173. 11 fly tour spots added; tour before/after identical, 0 of 11 spots
  changed. 6 regression spots with references; full suite 97/97 pass, 0
  changed pixels. Checks: exporter tests 121/121, regression tests 12/12,
  ctest 20/20. Evidence: runs/166-vault13depot-in-3d/ (probe-sheet.jpg,
  grid-*.jpg, hours-*.jpg, classic-2d-checks.jpg, walk-sheet.jpg, tour
  sheets, changed.jpg). A parallel glm sweep pass in the same worktree left
  its probe, walk and classic jpgs in the same folder; both passes agree.
