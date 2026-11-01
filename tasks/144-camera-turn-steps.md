---
id: 144
title: The camera turns in 10-degree steps, up to 30 by default, and edge scrolling follows it
milestone: M1
status: done
owner: claude
suggested: gpt
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

`[` and `]` turn the 3D camera 10 degrees a press. By default the turn
stops at 30 degrees either way. With the camera turned, scrolling the map
by the screen edge moves it toward the edge the mouse is at, as it does
unturned.

## Context

- The owner, 2026-11-01: "camera rotation in 10-degree steps, 30 in total
  by default"; "when the camera is turned, edge scrolling goes wrong, the
  direction changes".
- Today: `ce/src/scene/camera_turn.h` (`limitedCameraTurn`,
  `cameraTurnTarget`) turns 45 degrees a step and limits to +/-45 unless
  `[3d] camera_rotation_unlimited`; `scene_view.cc` (`turnCamera`,
  `updateCameraTurn`) animates it; the tests are in
  `ce/tests/settings_3d/settings_3d_test.cc`. The comment in
  `scene_view.h` says 45 too.
- The step and the default limit become named constants in one place.
  Unlimited still turns the full circle, in the same 10-degree steps, and
  landing back at a multiple of 360 still returns the game's own camera.
- Edge scrolling is the game's (`ce/src/game_mouse.cc`, the scroll
  directions, and `ce/src/map_edge.cc`): it moves the map along the
  unturned screen axes, so on a turned camera the map slides off at the
  turn angle. Rotate the requested screen direction back by the current
  turn before it becomes a map scroll (the nearest of the game's scroll
  directions, or a finer move if the 3D view can take one). Unturned, the
  scroll is exactly as now.
- Sprite sides turn with the camera (an existing setting); check they
  still pick the right side at the new 10, 20 and 30 degree stops.

## Acceptance

- Each press turns 10 degrees; the limited turn stops at +/-30; unlimited
  goes round the circle; back at 0 the game's camera returns.
- At 10, 20, 30 and -30 degrees, the mouse at each of the four edges and
  four corners scrolls the map toward that edge on screen.
- The settings tests cover the step, the limit and the unlimited circle.
- Evidence in `runs/144-camera-turn-steps/`: shots at 0, 10, 20 and 30
  degrees, and a short note per edge of where the map moved at 30.

## Log

- 2026-11-01 Cut from the owner's live find.

### 2026-11-01 claude
Done: step 10 and limit 30 are `kCameraTurnStep` and `kCameraTurnLimit` in `ce/src/scene/camera_turn.h`. Edge scroll on a turned camera goes through `cameraTurnScrollStep` (same header), called from `gameMouseHandleScrolling`: it rotates the edge direction by the turn into the game's eight scroll directions. Tests are in `ce/tests/settings_3d/settings_3d_test.cc`. Sprite sides read the camera angle itself, so they follow the new stops without a change.
Next: the owner checks live that each edge moves the map toward it at 10, 20 and 30 degrees (see `runs/144-camera-turn-steps/README.md`).
Check: `ctest --test-dir build --output-on-failure`.
