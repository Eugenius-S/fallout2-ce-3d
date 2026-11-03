---
id: 170
title: The city, cave and mountain encounters in 3D, every map stands and looks clean
milestone: M1
status: todo
owner: none
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Every map of the city, cave and mountain encounters exports, loads in the game in 3D and looks as clean as
Klamath: in the game camera and the fly camera, at morning, noon and
evening, every elevation. One of the areas of the all-maps sweep (136 Arroyo,
then 158 to 171 in the game's order).

## Context

- The maps, from the game's own `data/maps.txt` in `master.dat`:
  city1 city2 city3 city4 city5 city6 city7 city8 cave0 cave1 cave2 cave3 cave4 cave5 cave6 cave7 mountn1 mountn2 mountn3 mountn4 mountn5 mountn6. Add them to `MAP_GROUPS` in `tools/scene_export/export.py` as
  `enccitycave`. If `maps.txt` lists more maps for this area, take them too.
- What is here: Random encounter maps: ruined streets, caves, mountain paths. Expect art the exporter has not seen yet.
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
   `runs/170-enccitycave-in-3d/finds/`, the likely cause).
4. Group the finds by cause, fix the most visible group on its own branch
   `task/170-<short-cause>` from current origin/main, one PR titled
   `170: <cause>` with before | after crops. Mark the finds it closes.
5. Back to 4; when the list is empty, sweep again (3).

The first PR is the group plus the tour and regression spots, so the area
plays in 3D even before every find is fixed.

## Acceptance

- `export.py --maps enccitycave` exports every map without errors.
- Walking into each map in the game keeps 3D on.
- A final sweep finds no new defect; the Log holds every find marked fixed
  (PR number), owner, or open with a reason.
- Each fix PR: before | after crops, ctest, the regression and the fly tour
  against origin/main.

## Out of scope

The interface, menus and text, the sky, new 3D models from scratch, maps of
other areas (each has its own task).

## Log
