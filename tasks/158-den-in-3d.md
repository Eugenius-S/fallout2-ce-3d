---
id: 158
title: The Den in 3D, every map stands and looks clean
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

Every map of the Den exports, loads in the game in 3D and looks as clean as
Klamath: in the game camera and the fly camera, at morning, noon and
evening, every elevation. One of the areas of the all-maps sweep (136 Arroyo,
then 158 to 171 in the game's order).

## Context

- The maps (the game's map files, checked against `runs/p1-listmaps.log`):
  denbus1 denbus2 denres1 dnslvrun. Add them to `MAP_GROUPS` in `tools/scene_export/export.py` as
  `den`. If the game holds more maps for this area, take them too.
- What is here: Den is a slum: shacks, scrap walls, a church, the slave run's open ground. Expect art the exporter has not seen yet.
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
   `runs/158-den-in-3d/finds/`, the likely cause).
4. Group the finds by cause, fix the most visible group on its own branch
   `task/158-<short-cause>` from current origin/main, one PR titled
   `158: <cause>` with before | after crops. Mark the finds it closes.
5. Back to 4; when the list is empty, sweep again (3).

The first PR is the group plus the tour and regression spots, so the area
plays in 3D even before every find is fixed.

## Acceptance

- `export.py --maps den` exports every map without errors.
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
Done: the `den` group completes with dnslvrun (denbus1, denbus2, denres1,
dnslvrun; 10 elevations export clean). 20 fly tour spots (entrances plus the
landmarks from a 50-point 2D grid probe: the shop block, the upstairs
apartment, the shack interior, the Cat's Paw sign, the second storey, the
residential block, the fire pit, the slave camp garden, the corn field) and
9 regression spots with references from this build.
Find: denbus2 elevation 1 hung the dev travel, with and without the still
fixture. Cause: `applyElevationChange` finds no standable hex near the dude
or the entrance on the small second-storey floor, and the watcher waited
forever. Fixed in PR #242 (nearest free tile of the whole elevation, a clear
line and a settle when the elevation stays unreachable).
Find: denbus2 e1 looks like floating floor patches with no walls in the fly
view; the game's own 2D view of the same tile shows the same patches, so it
is the map, not the exporter. Not a find.
Sweep: the fly tour at every spot (3 turns), hours 8/12/18 on every
entrance, the hour sheets for the street, brothel and slave run views; a new
game walks denbus1 -> denres1 -> denbus2 -> dnslvrun in one run, all
scene-gl. tools/regression.ps1: 25 of 25 spots, 0 changed pixels. No open
visual finds; the task closes.
Note: the denbus2-e1 regression spot needs PR #242 (without it the capture
hangs); the other 8 den spots and the Klamath 16 do not.
Evidence: runs/158-den-in-3d/ (tour and hour sheets).
Check: `py -3 tools/scene_export/export.py --game-root <main>/game --maps den
--cache-root build/scene-cache`; `py -3 tools/tour/tour.py --before
build/scene-cache --maps denbus1,denbus2,denres1,dnslvrun`;
`tools/regression.ps1` (25 spots pass).
