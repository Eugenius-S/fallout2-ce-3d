---
id: 164
title: Redding in 3D, every map stands and looks clean
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
tier: 3
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
4. Since 2026-11-04 (the owner): the fixes go by kind, not by area. Move
   each find to its batch (189 walls, 190 roofs, 191 furniture and props,
   192 vehicles, 173 what cannot be built) and fix it there. A crash or a
   place the player cannot get past is fixed here at once. The old loop
   below stays for finds that fit no batch: fix the most visible group on its own branch
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

### 2026-11-04 gpt

Done: added the `redding` map group (`reddown`, `redment`, `reddtun`,
`redmtun`, `redwame`, `redwan1`). Exported all 7 elevations with no warnings.
Added 14 fly-tour points, two per map/elevation, and 7 regression points,
including both Wanamingo mine levels. The current-build game views and
hourly sweep are in `runs/164-redding-in-3d/`.

Sweep: every fly-tour point was captured at turns -45, 0 and +45 against a
fresh `origin/main` cache at `db16a224`: all 42 views per side show full
`scene-gl`, and 0 of 14 points changed pixels. The 7 map/elevation entrances
were captured at hours 8, 12 and 18 with sun shadows on; all 21 sidecars show
full `scene-gl`, 1920x1080, non-empty images. Direct sun changes the outdoor
maps; cave maps remain unchanged across the hours. A single background run
transitioned `reddown -> redment -> reddtun -> redmtun -> redwame -> redwan1
e0 -> redwan1 e1`, with 3D enabled on every map.

Checks: Release build passed; CTest 20/20; scene-export unit tests 121/121;
the 7 new regression points each passed with 0 changed pixels. JSON validation,
PowerShell parsing, public-content and runs-size checks passed. The first
fly-tour call exposed that nested Windows PowerShell 5.1 did not auto-load
`Microsoft.PowerShell.Utility`; `dev-snapshot.ps1` now imports it explicitly.
No map-specific visual defects found in this sweep.

Evidence: `current-build-game-views.jpg`, `hour-sweep-shadows.jpg`, and the
origin-main fly-tour sheets under `runs/164-redding-in-3d/`.
Next: no open finds; PR is ready for CI review.

### 2026-11-04 gpt follow-up

Done: expanded the visual sweep to all 14 fly-tour spots, all three camera
turns and hours 8, 12 and 18. The 126 runtime frames are in
`runs/164-redding-in-3d/multi-point-hour-sweep/`; all sidecars confirm the
requested map/elevation, full `scene-gl`, 1920x1080 and the requested game
time. All frames are non-empty, and every stop changes across its three turns.
The three hour sheets were inspected; no new scene defects were found.

The expanded sweep exposed one invalid reference point: `redment` tile 27012
was outside the authored map clip and produced a flat background. Replaced it
with tile 24912, which shows the town overview. The updated fly tour against
origin/main shows 0 of 14 changed spots, and both redment points now show
content in all turns. The refreshed comparison is in
`runs/164-redding-in-3d/fly-tour-origin-main/`.

Next: commit this correction, push it, and wait for CI on the new SHA.

### 2026-11-04 gpt merge

Done: merged latest `origin/main` (`2bcf027b`). The main-side PR added Redding
evidence files but left the exporter map group, tour points, and task status
out of the main tree, so this branch still carries the implementation. Two
add/add conflicts were shared regression sidecars: their PNG object hashes
were identical in both trees. The JSON files differed only in capture run,
CE/executable provenance and timestamps; kept main's sidecars and all main
changes, including its additional Wanamingo reference. The seven branch
regression spots and fourteen updated tour spots remain intact.

Checks after merge: Release build passed; CTest 20/20; scene-export unit tests
121/121; regression unit tests 12/12. Rechecking all seven Redding references
on the merged branch before push.

Next: commit and push the merge, then wait for CI on the new SHA.
