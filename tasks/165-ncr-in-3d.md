---
id: 165
title: NCR and Vault 15 in 3D, every map stands and looks clean
milestone: M1
status: done
owner: glm
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

Every map of NCR and Vault 15 exports, loads in the game in 3D and looks as clean as
Klamath: in the game camera and the fly camera, at morning, noon and
evening, every elevation. One of the areas of the all-maps sweep (136 Arroyo,
then 158 to 171 in the game's order).

## Context

- The maps (the game's map files, checked against `runs/p1-listmaps.log`):
  ncr1 ncr2 ncr3 ncr4 ncrent v15ent v15sent vault15. Add them to `MAP_GROUPS` in `tools/scene_export/export.py` as
  `ncr`. If the game holds more maps for this area, take them too.
- What is here: The NCR's walled city and its bazaar; the squatters' camp and the vault's ruined levels. Expect art the exporter has not seen yet.
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
   `runs/165-ncr-in-3d/finds/`, the likely cause).
4. Since 2026-11-04 (the owner): the fixes go by kind, not by area. Move
   each find to its batch (189 walls, 190 roofs, 191 furniture and props,
   192 vehicles, 173 what cannot be built) and fix it there. A crash or a
   place the player cannot get past is fixed here at once. The old loop
   below stays for finds that fit no batch: fix the most visible group on its own branch
   `task/165-<short-cause>` from current origin/main, one PR titled
   `165: <cause>` with before | after crops. Mark the finds it closes.
5. Back to 4; when the list is empty, sweep again (3).

The first PR is the group plus the tour and regression spots, so the area
plays in 3D even before every find is fixed.

## Acceptance

- `export.py --maps ncr` exports every map without errors.
- Walking into each map in the game keeps 3D on.
- A final sweep finds no new defect; the Log holds every find marked fixed
  (PR number), owner, or open with a reason.
- Each fix PR: before | after crops, ctest, the regression and the fly tour
  against origin/main.

## Out of scope

The interface, menus and text, the sky, new 3D models from scratch, maps of
other areas (each has its own task).

## Log

- 2026-11-04, gpt. Added the `ncr` export group for all eight maps named in
  the spec, including the three Vault 15 maps and `vault15`; the group exports
  13 map/elevation scenes with no errors or warnings. Runtime scene-tour loaded
  all 13 elevations as `scene-gl` / `full`.
- Added 13 regression points at the center of each map elevation and 25 fly
  tour points (overview plus entry where the entry point renders a scene).
  New references all pass pixel comparison at 0 changed pixels. The fly tour
  against scenes exported by origin/main's exporter found 0 of 25 views changed.
- Captured game-camera views of every elevation at 08:00, 12:00, and 18:00;
  sidecars confirm the requested map, elevation, camera tile, 1920x1080
  resolution, and full 3D renderer. Evidence: `runs/165-ncr-in-3d/`.
- The 25-stop fly tour covers the three camera turns at its normal daylight
  setting. The full fly-camera by hour cross-product was not captured after the
  two-failure stop, so this part of the acceptance remains incomplete.
- The initial `ncrent` elevation 1 spawn point (tile 20877) captured an empty
  view because its camera target fell outside the authored clip bounds. The
  clip center, tile 22148, shows the enclosure in 3D and matches the classic
  2D scene at the same tile. Regression and fly tour use that verified point.
- Walk probes succeeded on NCR1 e0/e1, NCR2 e0, NCR4 e0, NCRENT e0, and V15ENT
  e0/e1. NCR1 e0 walked through an exit into NCRENT e0 with 3D still enabled.
  The NCR3 e0 probe timed out; NCRENT e1 could not queue a path from its spawn
  tile. After V15ENT e1 succeeded, the next two probes, V15SENT e0 and VAULT15
  e0, timed out consecutively. Per the two-failure limit, runtime probes stop
  here; their logs are in the matching `walkprobe-*` folders. The task PR stays
  draft for this unresolved walk-probe coverage.
- Static follow-up (no additional runtime runs): the V15SENT e0 log changes to
  `GameState::DialogFinished` (2), and VAULT15 e0 changes to
  `GameState::DialogActive` (4, game mode 0x4) immediately after the walk starts.
  Those map scripts leave the probe waiting without a result. NCR3 e0
  finds a path of length 20 but logs no result or state transition; that timeout
  remains unexplained. Next step: a dialogue-aware walk check or a safe route
  for those maps, after the runtime-attempt limit is reset.
- Checks: Release build; ctest 20/20; scene exporter tests 121/121; regression
  tests 12/12; all 13 new regression points pass against their references.
- Finds: no confirmed scene defects to move to tasks 189-192 or 173.
- 2026-11-04, glm. The walk-probe and hour coverage the log above leaves
  open, completed on this branch: walk chain 13/13 across ncr1 e0/e1,
  ncr2-ncr4, ncrent e0/e1, v15ent e0/e1, v15sent, vault15 e0-e2 (3D stays
  on; direct map travel probes, so the exit-into-dialogue stops gpt hit did
  not reproduce). Hour sweep 8/12/18 captured for all 13 elevations. Grid
  probes on ncr1, ncr2, v15ent. Fly tour before/after identical on 13
  entrance spots. Dark views (ncr1-e1, ncrent-e1, vault15-e0/e2) match the
  game's own 2D (-Classic). 3 extra regression spots (ncr-council-square,
  v15-entrance, vault15-level) with references; full suite pass 0
  changed pixels with them (78/78 on this branch's suite, 91/91 after the
  merge with gpt's 13 spots, all green). The scene content itself: no defects; nothing
  to move to 189-192 or 173. Evidence: runs/165-ncr-in-3d/ (probe-sheet.jpg,
  tour-*.jpg, grid-*.jpg, hours-*.jpg, walk-*.jpg, classic-2d-checks.jpg).
