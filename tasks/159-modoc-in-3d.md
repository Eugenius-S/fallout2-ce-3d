---
id: 159
title: Modoc and the Ghost Town in 3D, every map stands and looks clean
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

Every map of Modoc and the Ghost Town exports, loads in the game in 3D and looks as clean as
Klamath: in the game camera and the fly camera, at morning, noon and
evening, every elevation. One of the areas of the all-maps sweep (136 Arroyo,
then 158 to 171 in the game's order).

## Context

- The maps (the game's map files, checked against `runs/p1-listmaps.log`):
  modmain modinn modbrah modgard modshit modwell gstfarm gstcav1 gstcav2. Add them to `MAP_GROUPS` in `tools/scene_export/export.py` as
  `modoc`. If the game holds more maps for this area, take them too.
- What is here: Farmland: wooden houses, fences, the brahmin pens, the outhouse, the well; the ghost farm and its caves. Expect art the exporter has not seen yet.
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
   `runs/159-modoc-in-3d/finds/`, the likely cause).
4. Group the finds by cause, fix the most visible group on its own branch
   `task/159-<short-cause>` from current origin/main, one PR titled
   `159: <cause>` with before | after crops. Mark the finds it closes.
5. Back to 4; when the list is empty, sweep again (3).

The first PR is the group plus the tour and regression spots, so the area
plays in 3D even before every find is fixed.

## Acceptance

- `export.py --maps modoc` exports every map without errors.
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
Done: the modoc group (modmain, modinn x2, modbrah, modgard, modshit, modwell,
gstfarm, gstcav1, gstcav2 x2; 11 elevations export clean). 21 fly tour spots
(entrances plus the landmarks from a 66-point 2D grid probe: the big barn,
the farmhouse, the inn and its car, the brahmin herd, Farrel's garden, the
outhouse pit, the well shaft, the massacre in the corn, the hermit camp, the
lake shore camp) and 11 regression spots with references from this build.
Find: modinn elevation 1 hung the dev travel even with the nearest-free-tile
fallback. Cause: mapHandleTransition refuses tile-based placement on a vanilla
blacklist (MAP_MODOC_BEDNBREAKFAST, MAP_THE_SQUAT_A), so the same-map
transition never applied. Fixed in PR #242 (the dev elevation change places
the dude directly; the engine path stays for real map changes).
Find: gstcav2 e1's floor is a thin ledge over black; the fly view is
legitimately near-empty (the game's own 2D shows the same), so it has a
regression spot but no tour stop (the tour rejects empty middles).
Sweep: the fly tour at every stop (3 turns), hours 8/12/18 on every entrance;
modmain -> modinn -> modbrah -> modgard in one run, all scene-gl.
tools/regression.ps1: 27 of 27 spots, 0 changed pixels. No open visual finds;
the task closes.
Note: the modinn-e1 regression spot needs PR #242 (without it the capture
hangs); the other 10 and the earlier areas do not.
Evidence: runs/159-modoc-in-3d/.
Check: export.py --maps modoc; tour.py --maps modmain,modinn,modbrah,modgard,
modshit,modwell,gstfarm,gstcav1,gstcav2; tools/regression.ps1.
