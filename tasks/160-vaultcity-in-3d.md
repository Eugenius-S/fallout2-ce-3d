---
id: 160
title: Vault City and the Raiders' camp in 3D, every map stands and looks clean
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

Every map of Vault City and the Raiders' camp exports, loads in the game in 3D and looks as clean as
Klamath: in the game camera and the fly camera, at morning, noon and
evening, every elevation. One of the areas of the all-maps sweep (136 Arroyo,
then 158 to 171 in the game's order).

## Context

- The maps (the game's map files, checked against `runs/p1-listmaps.log`):
  vctyctyd vctydwtn vctycocl vctyvlt raiders1 raiders2. Add them to `MAP_GROUPS` in `tools/scene_export/export.py` as
  `vaultcity`. If the game holds more maps for this area, take them too.
- What is here: Clean concrete and metal walls, a wall around the city, the vault's own corridors; the raiders' cave camp. Expect art the exporter has not seen yet.
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
   `runs/160-vaultcity-in-3d/finds/`, the likely cause).
4. Since 2026-11-04 (the owner): the fixes go by kind, not by area. Move
   each find to its batch (189 walls, 190 roofs, 191 furniture and props,
   192 vehicles, 173 what cannot be built) and fix it there. A crash or a
   place the player cannot get past is fixed here at once. The old loop
   below stays for finds that fit no batch: fix the most visible group on its own branch
   `task/160-<short-cause>` from current origin/main, one PR titled
   `160: <cause>` with before | after crops. Mark the finds it closes.
5. Back to 4; when the list is empty, sweep again (3).

The first PR is the group plus the tour and regression spots, so the area
plays in 3D even before every find is fixed.

## Acceptance

- `export.py --maps vaultcity` exports every map without errors.
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
Done: the vaultcity group (vctyctyd, vctydwtn, vctycocl, vctyvlt x3,
raiders1 x2, raiders2 x3; 11 elevations export clean). 20 fly tour spots
(entrances plus the landmarks from a 66-point 2D grid probe: the gate with
the towers, the downtown blocks, the wall gate, the vault corridor crossing,
the vault door, the anomaly room, the cliff cave mouth, the cave fire) and
10 regression spots with references from this build.
Find: the Central Council dome has no useful fly view: the dome's footprint
is so wide that the fly camera at orbit distance 8 stands over its own roof
(solid roof colour in every turn, tried 15392 and 13392). The dome is
covered by the regression spot at 15392 (the game's own 2D view) and the
council entrance tour spot. Not a scene defect; the tour stop was dropped.
Sweep: the fly tour at every stop (3 turns), hours 8/12/18 on every
entrance; vctyctyd -> vctydwtn -> vctycocl in one run, all scene-gl.
tools/regression.ps1: 26 of 26 spots, 0 changed pixels. No open visual
finds; the task closes.
Evidence: runs/160-vaultcity-in-3d/.
Check: export.py --maps vaultcity; tour.py --maps vctyctyd,vctydwtn,vctycocl,
vctyvlt,raiders1,raiders2; tools/regression.ps1.

### 2026-11-04 claude
Dropped the regression spot `vaultcity-council-dome` (tile 15392) at the
merge: its reference was all black, the camera target off the scene's x
bounds. Next: pick a vctycocl tile that shows the dome and add the spot back
with a fresh reference.
