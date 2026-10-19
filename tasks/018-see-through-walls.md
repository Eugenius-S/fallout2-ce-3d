---
id: 018
title: See-through walls around the hero
milestone: M1
status: todo
owner: none
suggested: claude
claude-only: yes
user-check: yes
depends-on: [013]
reviewed: no
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
