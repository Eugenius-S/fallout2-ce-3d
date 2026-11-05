---
id: 168
title: San Francisco and Navarro in 3D, every map stands and looks clean
milestone: M1
status: in-progress
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

Every map of San Francisco and Navarro exports, loads in the game in 3D and looks as clean as
Klamath: in the game camera and the fly camera, at morning, noon and
evening, every elevation. One of the areas of the all-maps sweep (136 Arroyo,
then 158 to 171 in the game's order).

## Context

- Owner priority, do these first (the owner, 2026-11-04, the docks from the
  fly camera, `runs/inbox/2026-11-04-sf-docks-sea-and-tanker.webp`: the dark
  water stops at the edge of the map tiles with flat brown void beyond, and
  the tanker's side is a slanted sliver floating above the pier):
  1. The sea fills the whole horizon: the water plane goes past the map to
     the left, to the right and into the distance, no brown void anywhere
     round the docks. A general rule: a map edge that is water continues as
     water.
  2. The tanker's side stands as one vertical plane, perpendicular to the
     water, touching the pier with no gap. It belongs to 173's plane
     fallback for big hull art, so it works for any ship art, not only this
     tanker.
- The maps (the game's map files, checked against `runs/p1-listmaps.log`):
  sfdock sfchina sfchina2 sftanker sfshutl1 sfshutl2 sfelronb navarro. Add them to `MAP_GROUPS` in `tools/scene_export/export.py` as
  `sanfran`. If the game holds more maps for this area, take them too.
- What is here: Chinatown's houses and temple, the docks, the tanker's decks, the shuttle, the Elronologists' base; the Navarro base. Expect art the exporter has not seen yet.
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
   `runs/168-sanfran-in-3d/finds/`, the likely cause).
4. Since 2026-11-04 (the owner): the fixes go by kind, not by area. Move
   each find to its batch (189 walls, 190 roofs, 191 furniture and props,
   192 vehicles, 173 what cannot be built) and fix it there. A crash or a
   place the player cannot get past is fixed here at once. The old loop
   below stays for finds that fit no batch: fix the most visible group on its own branch
   `task/168-<short-cause>` from current origin/main, one PR titled
   `168: <cause>` with before | after crops. Mark the finds it closes.
5. Back to 4; when the list is empty, sweep again (3).

The first PR is the group plus the tour and regression spots, so the area
plays in 3D even before every find is fixed.

## Acceptance

- `export.py --maps sanfran` exports every map without errors.
- Walking into each map in the game keeps 3D on.
- A final sweep finds no new defect; the Log holds every find marked fixed
  (PR number), owner, or open with a reason.
- Each fix PR: before | after crops, ctest, the regression and the fly tour
  against origin/main.

## Out of scope

The interface, menus and text, the sky, new 3D models from scratch, maps of
other areas (each has its own task).

## Log

- 2026-11-05, glm. Group `sanfran` = sfdock sfchina sfchina2 sftanker
  sfshutl1 sfshutl2 sfelronb navarro; 14 scene elevations exported clean
  (sfdock e0, sfchina e0, sfchina2 e0-e2, sftanker e0-e2, sfshutl1 e0,
  sfshutl2 e0, sfelronb e0, navarro e0-e2). No exporter errors or warnings.
- Entrance probes 14/14 (tiles: sfdock 20100, sfchina 26504, sfchina2
  26504/20133/24722, sftanker 27701 x2 / 23490, sfshutl1 26311, sfshutl2
  20695, sfelronb 23155, navarro 28694 x3). No hangs, no travel failures.
- Fly tour 14 spots, before/after identical everywhere ("0 of 14 spots
  changed"). Hour sweep 8/12/18 on all 14: outdoor ambient shifts with the
  hour, interiors stable. Dark views checked: sftanker-e1's black deck
  matches the game's own 2D (`-Classic`, tile 27701) — the map's own
  lighting, not a defect.
- Walk chain sfdock -> sfchina -> sfchina2 e0-e2 -> sftanker e0-e2 ->
  sfshutl1 -> sfshutl2 -> sfelronb -> navarro e0-e2: 14/14 captures, 3D
  stays on.
- Regression: full suite 100 spots, 0 changed pixels (3 new spots:
  sanfran-docks 20100, sanfran-tanker-deck 27701, navarro-base 28694).
  One reference regenerated with an explanation: depot-gatehouse (a 166
  spot whose reference predates #274's multipart-object rule) re-captured
  on the current renderer, a 6-pixel multipart edge shift, strictly 0 after.
- Finds (open, the owner's priorities, both belong in the batch 173 plane
  fallback per this task's Context):
  1. sfdock: the water stops at the map tiles' edge; flat brown void beyond
     (probe-sfdock-e0.jpg, tour sheet sfdock-e0.jpg). General rule to build:
     a map edge that is water continues as water past the edge.
  2. sftanker / sfdock: the tanker's hull has no standing side — the deck
     floats over void and from the docks the side is a slanted sliver
     (probe-sftanker-e0.jpg). General rule to build: big hull art stands as
     one vertical plane perpendicular to the water, touching the pier.
- Scene content otherwise: no defects; nothing else to move to 189-192 or
  173. The task stays open until the two rules above land.
- Evidence: runs/168-sanfran-in-3d/ (probe shots, tour sheets, hour sheets,
  walk shots, the sftanker 2D comparison).
