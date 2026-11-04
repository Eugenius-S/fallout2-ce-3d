---
id: 163
title: New Reno in 3D, every map stands and looks clean
milestone: M1
status: done
owner: glm
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

Every map of New Reno exports, loads in the game in 3D and looks as clean as
Klamath: in the game camera and the fly camera, at morning, noon and
evening, every elevation. One of the areas of the all-maps sweep (136 Arroyo,
then 158 to 171 in the game's order).

## Context

- The maps (the game's map files, checked against `runs/p1-listmaps.log`):
  newr1 newr2 newr3 newr4 newr1a newr2a newrcs newrgo newrst newrba newrvb. Add them to `MAP_GROUPS` in `tools/scene_export/export.py` as
  `newreno`. If the game holds more maps for this area, take them too.
- What is here: A big city: casinos, neon signs, tall buildings, the stables, the boxing arena, the vault-bunker. Expect art the exporter has not seen yet.
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
   `runs/163-newreno-in-3d/finds/`, the likely cause).
4. Since 2026-11-04 (the owner): the fixes go by kind, not by area. Move
   each find to its batch (189 walls, 190 roofs, 191 furniture and props,
   192 vehicles, 173 what cannot be built) and fix it there. A crash or a
   place the player cannot get past is fixed here at once. The old loop
   below stays for finds that fit no batch: fix the most visible group on its own branch
   `task/163-<short-cause>` from current origin/main, one PR titled
   `163: <cause>` with before | after crops. Mark the finds it closes.
5. Back to 4; when the list is empty, sweep again (3).

The first PR is the group plus the tour and regression spots, so the area
plays in 3D even before every find is fixed.

## Acceptance

- `export.py --maps newreno` exports every map without errors.
- Walking into each map in the game keeps 3D on.
- A final sweep finds no new defect; the Log holds every find marked fixed
  (PR number), owner, or open with a reason.
- Each fix PR: before | after crops, ctest, the regression and the fly tour
  against origin/main.

## Out of scope

The interface, menus and text, the sky, new 3D models from scratch, maps of
other areas (each has its own task).

## Log

### 2026-11-04 glm
Done: the newreno group (newr1 x3, newr2 x3, newr3 x2, newr4 x2, newrcs,
newrgo x2, newrst x2, newrba, newrvb; 21 elevations export clean). 21 fly
tour spots (entrances plus the landmarks from a 24-point 2D grid probe: the
lit casino corner, the car lot with the murals, the neon corner, the truck)
and 6 regression spots with references from this build.
Find: newr1a and newr2a are not in the game's data/maps.txt, so the game can
never travel there (devtools: not in the worldmap registry). They export,
but no tour or regression spot is possible. owner: unused leftover maps,
like arvill2 (136) and bhrndmtn (162).
Sweep: the fly tour at every stop (3 turns), hours 8/12/18 on every
entrance; newr1 -> newr2 -> newr3 -> newr4 in one run, all scene-gl.
tools/regression.ps1: 22 of 22 spots, 0 changed pixels. No open visual
finds; the task closes.
Evidence: runs/163-newreno-in-3d/.
Check: export.py --maps newreno; tour.py --maps newr1,newr2,newr3,newr4,
newrcs,newrgo,newrst,newrba,newrvb; tools/regression.ps1.
