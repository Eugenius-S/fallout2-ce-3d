---
id: 015
title: Camera zoom
milestone: M1
status: doing
owner: glm
suggested: glm
claude-only: no
user-check: yes
depends-on: [014]
reviewed: no
---

## Goal

The player zooms the 3D view in and out with the mouse wheel.

## Context

- The 3D view uses the game's own projection (`ce/src/scene_renderer/game_view.h`,
  `GameView`); zoom 1 must stay exactly today's picture.
- Mouse picking (objects and floor), the floating text and the hex cursor all
  go through the same projection (`staticScreenFromGamePixel`, the scene
  shift for the overlay): zoom must be applied there once, so they keep
  matching.
- The bottom interface and windows are not zoomed.
- The wheel already scrolls lists in game windows; zoom only when the map view
  has the mouse and no window is open.

## Acceptance

- Wheel zooms around the screen center in smooth steps, between limits kept as
  `[3d]` settings (`zoom_min`, `zoom_max`, defaults about 0.5 and 2), with a
  menu label each; the zoom itself is kept between sessions as `zoom`.
- At any zoom: clicks walk to the hex under the cursor, objects are picked
  correctly, floating text and the hex cursor sit on their objects.
- Regression snapshots at zoom 1 unchanged.

## Out of scope

Camera rotation.

## Log
