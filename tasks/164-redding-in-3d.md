---
id: 164
title: Redding in 3D, every map stands and looks clean
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

Every map of Redding exports, loads in the game in 3D and looks as clean as
Klamath: in the game camera and the fly camera, at morning, noon and
evening, every elevation. One of the areas of the all-maps sweep (136 Arroyo,
then 158 to 171 in the game's order).

## Context

- The maps (the game's map files, checked against `runs/p1-listmaps.log`):
  reddown redment reddtun redmtun redwame redwan1. Add them to `MAP_GROUPS` in `tools/scene_export/export.py` as
  `redding`. If the game holds more maps for this area, take them too.
- What is here: A mining town: the downtown, the mine entrance, tunnels, the Wanamingo mine. Expect art the exporter has not seen yet.
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
   `runs/164-redding-in-3d/finds/`, the likely cause).
4. Group the finds by cause, fix the most visible group on its own branch
   `task/164-<short-cause>` from current origin/main, one PR titled
   `164: <cause>` with before | after crops. Mark the finds it closes.
5. Back to 4; when the list is empty, sweep again (3).

The first PR is the group plus the tour and regression spots, so the area
plays in 3D even before every find is fixed.

## Acceptance

- `export.py --maps redding` exports every map without errors.
- Walking into each map in the game keeps 3D on.
- A final sweep finds no new defect; the Log holds every find marked fixed
  (PR number), owner, or open with a reason.
- Each fix PR: before | after crops, ctest, the regression and the fly tour
  against origin/main.

## Out of scope

The interface, menus and text, the sky, new 3D models from scratch, maps of
other areas (each has its own task).

## Log

- 2026-11-04, glm. Group `redding` = reddown redment reddtun redmtun redwame
  redwan1 (elevations: all e0, redwan1 also e1). All 7 scenes exported clean,
  no exporter errors or warnings.
- Entrance probes 7/7 (tiles: reddown 19059, redment 26912, reddtun 20100,
  redmtun 20100, redwame 20275, redwan1 26893 e0+e1). No hangs, no travel
  failures; the elevation-blacklist and nearest-free-tile rules from 159/158
  cover everything.
- Grid probes on the two outdoor maps (reddown, redwame, 6 offsets each):
  roofs, lit windows, streets, tents, ruins all correct; black wedges are
  views past the map edge (normal).
- Fly tour 7 spots, before/after identical everywhere. Hour sweep 8/12/18 on
  all 7 spots: outdoor maps shift ambient with the hour, tunnels stable.
- Suspicious dark view redwan1-e1 checked against the game's own 2D
  (`-Classic`, same tile, same hour): equally dark there too — the mine's own
  lighting, not a defect.
- Walk chain reddown → redment → redwame → redwan1 e0 → e1 → reddtun →
  redmtun: 7/7 captures, 3D stays on.
- Regression: full suite pass, 0 changed pixels everywhere; 3 new spots
  (redding-downtown 19059, redding-mine-entrance 26912, redding-wanamingo
  26893 e1) generated against this branch, existing references untouched.
- Finds: 0. Nothing fixed, nothing given to the owner, nothing open.
- Evidence: runs/164-redding-in-3d/ (tour/grid/hours sheets, probe and walk
  shots, the 2D comparison).
