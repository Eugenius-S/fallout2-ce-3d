---
id: 018
title: See-through walls around the hero
milestone: M1
status: done
owner: claude
suggested: claude
claude-only: no
user-check: yes
depends-on: [013]
reviewed: yes
---

## Goal

Walls in front of the hero turn see-through around him, as in 2D, so he is
never lost behind a building.

## Context

- Task 013 does this for roofs: the egg mask (`gEgg`, 129x98; 0 keeps the
  surface, else its share in 128ths) applied in the scene shader (`uEgg`,
  `scene_renderer/roofs.cc`), positioned by `scene_view.cc`
  (`updateRoofsFromGame`).
- In 2D the engine applies the egg to wall objects in front of the hero
  (`_obj_render_object`, `object.cc` around line 1930: which walls count as
  in front). In 3D walls are drawn before objects and write depth, so a
  blended wall fragment would still hide the hero behind it: the wall pass
  needs to leave such fragments out of the depth buffer, or the hero and
  objects behind need a second pass.

## Acceptance

- Behind a building's front wall the hero shows through the same soft circle
  as in 2D; walls behind the hero are not cut.
- Regression references updated with before and after images.

## Out of scope

Roofs (done in 013).

## Log

### 2026-10-19 claude
Done: walls and scenery the hero stands behind are cut by the egg as in 2D.
The rule which objects (`_obj_render_object`) is now one function,
`objectSeeThroughAroundDude` (`object.cc`), used by 2D and 3D. walls.bin
(version 3, exporter 1.10.0) names the map object of every face; each frame
the scene marks the rows the rule picks, and the shader draws walls and
cards without the egg's part first and blends that part over the hero last.
Test runs can read a worktree's own scene cache (`F2CE_SCENE_CACHE`,
`export.py --cache-root`), so a new format does not break the owner's
running game.
Evidence: `runs/018-see-through-walls/` (`cave-2d-before-after.png`: 2D,
before, after: the mine beam over the hero is cut; `store-2d-3d-parallel.png`:
inside the store, 2D and 3D without perspective). Three cave references
retaken; the other 13 spots are unchanged. Where 2D shows black through the
hole and 3D the floor or walls behind, the difference is the game's light
(task 020).
Next: the owner checks it in the game.
Check: `tools/regression.ps1` (16 pass); `ctest --test-dir build` (8/8);
exporter tests 38 OK.

### 2026-10-20 claude
Done: its acceptance held at the merge; it stayed open only for the owner's
look, which blocks nothing (AGENTS.md, "Nobody waits"). What the owner
rejects in the game becomes a new task.
