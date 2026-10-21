---
id: 049
title: The see-through circle around the hero follows the zoom
milestone: M1
status: doing
owner: glm
suggested: glm
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The circle cut out of walls and roofs around the hero (the game's egg) has
the hero's size on screen at any zoom, as it has at zoom 1.

## Context

- The owner's find, 2026-10-21: zoomed in, the circle stays the game's size
  in screen pixels and cuts a small hole in the fence in front of the hero
  (`runs/049-egg-zoom/owner-egg-not-scaled.png`).
- The egg goes to the scene as a mask with its top-left corner on screen in
  game logical pixels (`setStaticEgg`, `ce/src/scene_renderer/static_scene.h`;
  `updateRoofsFromGame`, `ce/src/scene/scene_view.cc`). The zoom scales the
  game view around the screen center (`game_view.cc`), so the mask needs the
  same scale and the same center.
- The smooth zoom (042) changes the shown zoom every frame: use the shown one.

## Acceptance

- At zoom 0.5, 1 and 2 the circle sits on the hero and has the hero's size;
  snapshots in `runs/049-egg-zoom/`.
- At zoom 1 nothing changes: the regression passes as it is.

## Log
