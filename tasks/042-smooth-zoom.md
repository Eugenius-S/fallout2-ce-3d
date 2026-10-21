---
id: 042
title: Smooth camera zoom with easing
milestone: M1
status: done
owner: glm
suggested: glm
claude-only: no
user-check: yes
depends-on: [015]
reviewed: yes
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

### 2026-10-21 glm
Done: the wheel only sets the target (`settings.scene3d.zoom`, clamped to
`zoom_min..zoom_max` as before) in `scene::adjustZoom`; the shown zoom eases
to it in `refreshForPresent` with a cubic ease-out over 0.2 s
(`kZoomGlideSeconds`, `shownZoom()` in ce/src/scene/scene_view.cc). A step in
mid-glide re-anchors from the value on screen, so quick wheel steps chain
into one continuous glide; with no glide running the shown zoom still follows
the stored one every frame, so a changed setting or the test runs' forced
zoom 1 applies live. Non-issue checked: `setCameraZoom` has no other callers.
How it feels: the first notch moves most of the way at once and settles
softly, no overshoot; a fast wheel spin glides smoothly to the last target
with no jerk between notches. The owner judges the feel after the merge.
Next: nothing.
Check: build, ctest 11/11, regression 16/16 (exit 0; klatoxcv-e1's tolerated
4 pixels as before).
