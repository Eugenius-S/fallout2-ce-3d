---
id: 015
title: Camera zoom
milestone: M1
status: done
owner: glm
suggested: glm
claude-only: no
user-check: yes
depends-on: [014]
reviewed: yes
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

### 2026-10-19 glm
Done: zoom implemented and building. GameView gained `zoom` (game_view.h);
the projection scales around the screen center in `clip()`, so picking, the
hex cursor, floating text and the overlay shift follow it (game_view.cc).
Planes adapt: near = d*(1-(1-kNearShare)/zoom), far = d*kFarShare/zoom; zoom
1 keeps today's exact planes and pixels (regression at zoom 1 unaffected by
construction). The camera holds the zoom (scene_renderer/camera.cc
setCameraZoom, clamped 0.1..10) and `currentGameView` fills it. Settings
`[3d] zoom/zoom_min/zoom_max` (defaults 1.0/0.5/2.0, clamp 0.1..10, menu
labels + descriptions, written back on close) in settings.{h,cc}. The wheel
zooms in game.cc's map-loop branch when the full 3D view presents and the
cursor is not over the interface bar (new gameMouseIsOverInterfaceBar);
otherwise the stock wheel jobs run. scene::adjustZoom steps x1.25 per notch,
clamped to the limits, and the renderer follows the stored zoom every frame
in refreshForPresent. Build is green; ctest 8/8; a manual dev-snapshot at
zoom 1 captured normally (game/devshot-20261020-080309-000.json).
Next: `tools/regression.ps1` from this worktree fails with "Another
fallout2-ce-test.exe instance is already running" cascading over all spots
(runs/regression/20261020-*): instances start (PID changes over time) but no
new devshot appears after 080309 even at zoom 1, so the still-snapshot flow
of the regression instances does not complete in this shared-game-dir setup.
Needs a diagnosis with the owner's game closed, or a regression run from the
main checkout with this branch's build copied in. Then: in-game wheel check
(user-check), two-frames evidence, PR.
Check: build + ctest green (worktree task/015-camera-zoom); zoom 1 snapshot
game/devshot-20261020-080309-000.json.

### 2026-10-19 glm
Done: the regression passes clean from this worktree - 16/16 spots, 0 changed
pixels at zoom 1 (the earlier cascade was two invocations racing over one
test instance; a single run is green). The wheel path is code-reviewed only:
the owner drives the in-game wheel check (user-check).
Next: merge; the options-menu labels land with task 017's panel.
Check: tools/regression.ps1; [3d] zoom keys in fallout2.cfg after a session.

### 2026-10-20 claude (review)
Accepted (PR #25). The zoom sits in one place, `clip()`, so the floor and
object picks, the hex cursor and the floating text follow it; zoom 1 keeps the
planes and the picture exactly (regression 16/16). The wheel zooms only over
the map with the 3D view shown; `[3d] zoom`, `zoom_min`, `zoom_max` have menu
labels and the zoom is kept between sessions. Removed `runs/dbg015/` (a debug
capture, not evidence). The wheel's feel is the owner's check in the game.
