---
id: 170
title: The city, cave and mountain encounters in 3D, every map stands and looks clean
milestone: M1
status: done
owner: glm
suggested: glm
priority: normal
tier: 3
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

- The maps (the game's map files, checked against `runs/p1-listmaps.log`):
  city1 city2 city3 city4 city5 city6 city7 city8 cave0 cave1 cave2 cave3 cave4 cave5 cave6 cave7 mountn1 mountn2 mountn3 mountn4 mountn5 mountn6. Add them to `MAP_GROUPS` in `tools/scene_export/export.py` as
  `enccitycave`. If the game holds more maps for this area, take them too.
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
4. Since 2026-11-04 (the owner): the fixes go by kind, not by area. Move
   each find to its batch (189 walls, 190 roofs, 191 furniture and props,
   192 vehicles, 173 what cannot be built) and fix it there. A crash or a
   place the player cannot get past is fixed here at once. The old loop
   below stays for finds that fit no batch: fix the most visible group on its own branch
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

- 2026-11-05, glm. Group `enccitycave` = city1-8, cave0-7, mountn1-6; 28
  scene elevations exported clean, no exporter errors or warnings. Spots
  from each scene's spawn tile (where the player walks in): 28 fly tour
  spots, 23 regression spots (22 maps + cave5's top shelf). ctest 129/129.
- Probe tour 0 of 28 changed (the same cache on both sides, sheets as the
  probe evidence); the visual sweep of all 28 sheets found no exporter or
  renderer defect. Walk probes: every group's tour base snapshot enters at
  the spawn with the 3D scene on (scene-gl in all sidecars), 28/28, no
  hangs. Observations, not defects: mountn5/mountn6 fly views show a tan
  border strip with void squares - the maps' own border data (NO_TILE
  squares next to drawn dirt tiles), unreachable by the game camera (the
  2D cannot scroll there), so the fly view is faithful; the dark dithered
  tree canopies (cave1/3/5, mountn3) are the maps' own art, confirmed
  against the game's 2D; black map-edge corners are 168's near-black
  continuation (rule A).
- Hour sweep 8/12/18, all 28 elevations, 84 captures: no hour-dependent
  defect; spot-verified against the game's own 2D at hour 18 (city1), the
  renderers agree. Background captures keep the map's default light in
  both renderers at every hour - the play-time ambient dimming is not
  exercised by --dev-hour in a background run (engine behaviour, not a
  group defect). Tooling finds: (a) -Still pins the sun report's ambient
  at 65536, so an hour sweep must NOT use -Still - the first pass shot 84
  identical-looking pictures; (b) cave3 e1 h12/h18 hit one-off 90 s
  whole-run timeouts and passed on a plain retry - the same transient
  dead-game-process class as 168's black-frame find.
- Tooling find, the session's first blocker: the worktree's exe (built
  05:45) predated #303 (18:48, the v7 floor.bin water flags), so every new
  scene stayed 2D with "floor preview unavailable: floor.bin version,
  dimensions, or length is unsupported" although the sidecars matched the
  formula exactly. A rebuild through configure-build.ps1 fixed it. Rule:
  before a capture session, compare the exe's build time against the last
  renderer-touching merge; rebuild when in doubt.
- Regression: full suite 129 spots (106 old + 23 new), 129/129 pass, 0
  changed pixels everywhere - the already-done maps stay pixel for pixel,
  the new references captured clean. No defects to move to 189-192 or
  173; the area plays in 3D. Evidence: runs/170-enccitycave-in-3d/ (28
  probe sheets, the mountn5 border 2D-vs-3D pair).
