---
id: 171
title: The special encounters and the rest in 3D, every map stands and looks clean
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

Every map of the special encounters and the rest exports, loads in the game in 3D and looks as clean as
Klamath: in the game camera and the fly camera, at morning, noon and
evening, every elevation. One of the areas of the all-maps sweep (136 Arroyo,
then 158 to 171 in the game's order).

## Context

- The maps (the game's map files, checked against `runs/p1-listmaps.log`):
  rndwhale rndtinwd rndbhead rndshutl rnduvilg rndbridg rndcafe rndholy1 rndholy2 rndforvr rndtoxic rndparih rndexcow rndbess encdet encdock encfite encgd encpres encrctr enctrp cowbomb cardesrt. Add them to `MAP_GROUPS` in `tools/scene_export/export.py` as
  `encspecial`. If the game holds more maps for this area, take them too.
- What is here: The special encounters (the whale, the cafe, the bridge, the shuttle and the rest), the fixed encounters, the car's map. Expect art the exporter has not seen yet.
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
   `runs/171-encspecial-in-3d/finds/`, the likely cause).
4. Since 2026-11-04 (the owner): the fixes go by kind, not by area. Move
   each find to its batch (189 walls, 190 roofs, 191 furniture and props,
   192 vehicles, 173 what cannot be built) and fix it there. A crash or a
   place the player cannot get past is fixed here at once. The old loop
   below stays for finds that fit no batch: fix the most visible group on its own branch
   `task/171-<short-cause>` from current origin/main, one PR titled
   `171: <cause>` with before | after crops. Mark the finds it closes.
5. Back to 4; when the list is empty, sweep again (3).

The first PR is the group plus the tour and regression spots, so the area
plays in 3D even before every find is fixed.

## Acceptance

- `export.py --maps encspecial` exports every map without errors.
- Walking into each map in the game keeps 3D on.
- A final sweep finds no new defect; the Log holds every find marked fixed
  (PR number), owner, or open with a reason.
- Each fix PR: before | after crops, ctest, the regression and the fly tour
  against origin/main.

## Out of scope

The interface, menus and text, the sky, new 3D models from scratch, maps of
other areas (each has its own task).

## Log

- 2026-11-05, glm. Group `encspecial` = rndwhale rndtinwd rndbhead
  rndshutl rnduvilg rndbridg rndcafe rndholy1 rndholy2 rndforvr rndtoxic
  rndparih rndexcow rndbess encdet encdock encfite encgd encpres encrctr
  enctrp cowbomb cardesrt (23 maps, 28 scene elevations); 0 export errors
  or warnings. ctest exporter suite 132/132.
- Find 1, fixed here: encfite e1/e2 refused to export ("invalid
  tiles.cells list") - they are void boxes, every square is the nothing
  tile (the raw grid000 value 1) and the atlas holds no cell; the
  renderer rejected tileCount 0 and the view policy asked for drawn
  floor. The rule, general for every map: a tile-less elevation exports
  with an empty atlas and keeps 3D on - the renderer fills the void as
  the sea rule continues the maps' own grid000 squares (the near-black
  plane on land), and the void is the ground (the game's own walkability
  decides where the hero steps); a drawn square over an empty atlas is
  still a broken scene. The sea fill's 6000-square cap now covers the
  whole grid, else a wholly void elevation would fill only part-way.
  The game's own enter script never lands on encfite e1/e2 in a dev run
  (it picks the fight room on e0), so the elevations are proven by the
  encode/decode round trip and the renderer path, not by a capture.
- Find 2, harness-side: cowbomb cannot be captured - the map's own
  script teleports to artemple at enter, the dev travel cannot stay on
  it, so the map has no tour or regression spot and its scene sits
  exported in the cache. The tour's check caught it as "shot at
  ARTEMPLE.MAP".
- Spots from the spawn tiles, four re-aimed at the objects' dense rows
  (rndwhale 17705, rndshutl 17899, rndbridg 19298, rndbess 13085 - the
  spawn-centre view showed bare dirt): 26 fly tour spots, 25 regression
  spots. Probe tour 0 of 25 changed; the visual sweep of all 25 sheets
  found no exporter or renderer defect. Observations, not defects:
  rndbess e1 is a whole-map void box (9993 grid000 squares, 7 drawn),
  so its fly view is the rule's near-black plane; the tan border strips
  (rndtinwd, rnduvilg) and the dark dithered tree canopies are the
  maps' own art. Walk probes: every group's base snapshot enters at the
  spawn with the 3D scene on (scene-gl in all sidecars), 25/25.
- Hour sweep 8/12/18 on all 25 elevations, 75 captures: no
  hour-dependent defect, all scene-gl.
- Regression: full suite 154 spots (129 old + 25 new), 154/154 pass, 0
  changed pixels - the already-done maps stay pixel for pixel, the new
  references captured clean. No defects to move to 189-192 or 173; the
  area plays in 3D. Evidence: runs/171-encspecial-in-3d/ (25 probe
  sheets, the encfite e0 2D mural).
- Tooling finds: (a) object tiles use the 200-wide hex row numbering
  (row = tile // 200, up to ~20200) - filtering or clustering them by
  the floor grid's 100-wide indexing silently drops half the map; (b) a
  serial-rule slip of mine: a sheets-regeneration tour ran while the
  full regression was re-exporting the same cache - stopped it before
  any damage (the shots it would have reused were taken before the
  regression started, and the sheets were rebuilt after it finished).
