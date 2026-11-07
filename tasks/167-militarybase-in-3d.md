---
id: 167
title: The Military Base in 3D, every map stands and looks clean
milestone: M1
status: done
owner: glm
suggested: glm
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

Every map of the Military Base exports, loads in the game in 3D and looks as clean as
Klamath: in the game camera and the fly camera, at morning, noon and
evening, every elevation. One of the areas of the all-maps sweep (136 Arroyo,
then 158 to 171 in the game's order).

## Context

- The maps (the game's map files, checked against `runs/p1-listmaps.log`):
  mbclose mbase12 mbase34. Add them to `MAP_GROUPS` in `tools/scene_export/export.py` as
  `militarybase`. If the game holds more maps for this area, take them too.
- What is here: The Mariposa base: the entrance on the surface, then metal and concrete corridors down several levels. Expect art the exporter has not seen yet.
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
   `runs/167-militarybase-in-3d/finds/`, the likely cause).
4. Since 2026-11-04 (the owner): the fixes go by kind, not by area. Move
   each find to its batch (189 walls, 190 roofs, 191 furniture and props,
   192 vehicles, 173 what cannot be built) and fix it there. A crash or a
   place the player cannot get past is fixed here at once. The old loop
   below stays for finds that fit no batch: fix the most visible group on its own branch
   `task/167-<short-cause>` from current origin/main, one PR titled
   `167: <cause>` with before | after crops. Mark the finds it closes.
5. Back to 4; when the list is empty, sweep again (3).

The first PR is the group plus the tour and regression spots, so the area
plays in 3D even before every find is fixed.

## Acceptance

- `export.py --maps militarybase` exports every map without errors.
- Walking into each map in the game keeps 3D on.
- A final sweep finds no new defect; the Log holds every find marked fixed
  (PR number), owner, or open with a reason.
- Each fix PR: before | after crops, ctest, the regression and the fly tour
  against origin/main.

## Out of scope

The interface, menus and text, the sky, new 3D models from scratch, maps of
other areas (each has its own task).

## Log


## Log

- 2026-11-05, glm. The militarybase group (mbclose e0, mbase12 e0-e1,
  mbase34 e0-e2; 6 elevations) exports clean into build/scene-cache, re-run
  after 174's exporter change landed on main (fresh fingerprints, no errors
  or warnings). Entrance stills, a grid probe on mbase12 e0, mbase34 e1,
  mbclose e0, and the 8/12/18 hour sweep on all 6 elevations show every
  corridor, the vat room's green glow, the surface sheds and fences
  standing. Walk probes 4/4 keep 3D on. The dark views (mbase12 e0, mbase34
  e0) match the game's own 2D picture (-Classic).
- Finds:
  1. mbase34 e1, tile 17131 (also 11131): a vertical beam below the glowing
     pool edge takes the pool's green light in 3D; the game's own view keeps
     that region unlit. Likely cause: the emissive pool light reaches cards
     the 2D lightmap leaves dark. Game and fly camera, all hours. Crop:
     finds/t17131-classic-vs-3d.jpg. Batch: 191 (props lighting).
  2. mbase34 elevation 2 is unreachable for the still fixture and the fly
     tour: the map's entrance elevation 2 has no free hex for the hero ("no
     free space" status), the game falls back to elevation 0, so the probe
     and hour shots labelled e2 actually show e0 (the probe sheet labels
     them). The scene itself exports fine and ships. This is a fixture
     limit, not a player blocker (the 2D game reaches e2 by the stairs and
     the 3D view follows the same rules). Batch: 173 for a fixture-side
     decision if the owner wants e2 pinned.
- Fixed here by a general rule in devtools (ce/src/devtools/devtools.cc):
  a --dev-map-tour stop the game cannot reach or one blocked by combat, a
  dialog or a movie no longer kills the whole tour; the tour snapshots
  where the game is, logs it and goes on. Before this, one unreachable stop
  aborted every later spot (mbase34 e2 aborted the militarybase tour).
- Tour: 5 spots (mbase34 e2 dropped as unreachable for the fixture), tour
  before/after identical, 0 of 5 spots changed. 3 regression spots with
  references (mbase-surface, mbase-corridors, mbase-vats); full suite 100
  spots, every spot pass (my run: 97 pass, 3 capture errors from two
  regression runners racing in one worktree; the rerun: 100/100 pass, 0
  changed pixels; depot-gatehouse sits at its 6-pixel tolerance in both
  runs). Checks: exporter tests 125/125 (174 added four), regression tests
  12/12, ctest 20/20. Evidence: runs/167-militarybase-in-3d/ (probe-sheet,
  grid-*, hours-*, classic-2d-checks, walk-sheet, tour sheets, changed.jpg,
  finds/).
- 2026-11-07: the owner ran the launcher bat themselves and gave the visual
  verdict: the glow (puddles, beams) is there and looks right. The bat stays
  in the launch hub folder; deleting it is allowed.
