---
id: 136
title: Arroyo in 3D, the game's opening area stands and looks clean
milestone: M1
status: done
owner: glm
suggested: glm
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

A new game starts in 3D. Every Arroyo map (artemple, arcaves,
arvillag, arvill2, arbridge, argarden, ardead, and any other map the game files list for Arroyo, all
elevations) exports, loads in the game in 3D and looks as clean as Klamath:
in the game camera and the fly camera, at morning, noon and evening. Today
scene data exists only for Klamath, so the first hour of the game is still
2D.

## Context

- `tools/scene_export/export.py` holds `MAP_GROUPS`. Add an `arroyo` group;
  take the map names from the game's own map list in `master.dat`, not from
  memory.
- Arroyo has terrain Klamath lacks: cave rock in the temple, stone walls,
  the village's huts and fences, the bridge, cliffs. Expect new shapes the
  exporter has never seen. Fix by general rules in the exporter or the
  renderer that hold for every map and object of that kind; no per-object,
  per-tile or per-map case. A defect no general rule fixes goes into the Log
  as "owner:".
- Everything that already makes Klamath clean (tasks 100-125, 129) must keep
  working: Klamath's regression and fly tour stay pixel for pixel or every
  change is explained.
- Export into your own cache (`build/scene-cache` or your worktree's game
  folder). Never write the owner's `game/mod-cache`.

## How to work

A loop, without waiting for anyone between rounds:
1. Export the group. Every export error or warning is a find.
2. Add fly tour spots and regression spots for each Arroyo map, so later
   tasks keep it clean.
3. Sweep: walk every map and camera, write each defect as one numbered line
   in this task's Log (map, tile or spot, camera, hour, a crop in
   `runs/136-arroyo-in-3d/finds/`, the likely cause).
4. Group the finds by cause, take the most visible group, fix it on its own
   branch `task/136-<short-cause>` from current origin/main with before |
   after crops, ctest, the Klamath regression and fly tour against
   origin/main. One PR per cause group, titled `136: <cause>`; mark the finds
   it closes in the Log.
5. Go back to 4. When the list is empty, sweep again (3). A defect that needs
   the owner's eye or a design decision goes into the Log as "owner:" and
   the loop moves on.

The first PR is the group itself plus the tour spots, so the maps can be
played in 3D even before every find is fixed.

## Acceptance

- `export.py --maps arroyo` exports every Arroyo map without errors.
- A new game from the main menu shows the Temple of Trials in 3D, and
  walking to the village keeps 3D on.
- Each fix PR: before | after crops, ctest passes, the Klamath regression is
  pixel for pixel or every changed pixel is explained, the fly tour changes
  are explained.
- The Log holds the full find list with each line marked fixed (PR number),
  owner, or open.

## Out of scope

The interface, menus, wallpapers and text, the sky (113), new 3D models of
landmark objects from scratch, maps outside Arroyo.

## Log

### 2026-11-04 glm
Done: the `arroyo` group in `tools/scene_export/export.py` (arcaves, arvillag,
arvill2, arbridge, argarden, ardead, artemple; the map files from the game,
arvill2 and ardead were not in my first maps.txt pass, they come from the
board's file list). All 9 elevations export clean into the worktree cache.
8 fly tour spots (tools/tour/spots.json) and 8 regression spots
(tools/regression/spots.json) at the entrances, with 16 reference files under
tools/regression/reference/ taken from this build. The first sweep found the
still-fixture movie hang: fixed in PR #240 (arvillag's map-enter script plays
a movie; the fixture froze the tickers the playback advances on).
Find: arvill2 is not in the game's data/maps.txt, so the game can never travel
there (devtools: "not in the worldmap registry"). It exports, but no tour or
regression spot is possible. owner: an unused leftover map, left alone.
Next: the grid sweep of every map (2D probes to place more spots), then the
fly sweep at morning, noon and evening, fixes by cause.
Check: `py -3 tools/scene_export/export.py --game-root <main>/game --maps
arroyo --cache-root build/scene-cache`; `py -3 tools/tour/tour.py --before
build/scene-cache --maps arcaves,arvillag,arbridge,argarden,ardead,artemple`;
`tools/regression.ps1` (24 spots pass, arroyo references new).

### 2026-11-04 glm (second round, task done)
Done: the sweep is clean, the task closes with no open visual find.
- 9 more fly tour spots at the landmarks found by the 48-point 2D grid probe
  (the stone head plaza, the rope bridge, the cave gate, the cave mouth, the
  corridor maze, the forest heart, the camp, the brahmin pen, the north huts);
  4 of them are regression spots too (references in the same PR).
- Fly views at hours 8, 12 and 18 on every entrance spot and every landmark
  spot: the sun and shadows move correctly on rock, huts, trees and the
  temple; caves stay dark inside; the black canopy look matches the game's
  own dark tree art (checked against the 2D view at the same tile).
- A new game walks artemple -> arcaves -> arvillag in one run, all three
  maps render scene-gl (devshot JSONs in the walk probe).
- tools/regression.ps1: 28 of 28 spots pass, 0 changed pixels each; the 16
  Klamath references untouched.
Find list: the still-fixture movie hang (fixed, PR #240); arvill2 exports but
is unreachable in the game (owner, above). No other finds; nothing open.
Evidence: runs/136-arroyo-in-3d/ (tour and hour sheets).
Check: `tools/regression.ps1` (28 spots pass); the walk probe and the hour
sweep commands live in the PR #241 description.

