---
id: 042
title: Smooth camera zoom with easing
milestone: M1
status: todo
owner: none
suggested: glm
claude-only: no
user-check: yes
depends-on: [015]
reviewed: no
---

## Goal

The mouse wheel zooms the 3D view smoothly: the zoom glides to its new value
with easing instead of jumping a step at a time (the owner's wish,
2026-10-20).

## Context

- The wheel calls `scene::adjustZoom(steps)` (`ce/src/scene/scene_view.cc`):
  each step multiplies the zoom by 1.25 within `[3d] zoom_min..zoom_max`,
  saves it in `settings.scene3d.zoom` and sets it at once with
  `scene_renderer::setCameraZoom` (`ce/src/scene_renderer/camera.cc`).
- The camera turn in the same file (`updateCameraTurn`, called every frame
  from the main loop) shows how a value glides frame by frame.
- Background test runs force zoom 1 (`devtools::applyFixedScreenOptions`).

## Acceptance

- The wheel sets a target zoom; the shown zoom eases to it (ease-out, about
  0.2 s), several quick wheel steps keep gliding without a jerk.
- The saved zoom is the target; the zoom limits hold.
- Regression 16/16 unchanged.
- A short note in the `Log` on how it feels (the owner looks after the merge).

## Out of scope

Zooming toward the mouse cursor.

## Log
