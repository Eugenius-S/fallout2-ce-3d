---
id: 150
title: The camera turn step eases in and out
milestone: M1
status: doing
owner: glm
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

One press of `[` or `]` turns the camera its 10 degrees smoothly: it starts
gently, speeds up and settles gently on the step (ease in, ease out), not
at one constant speed with a hard start and stop.

## Context

- The owner, 2026-11-01: "make the camera turn step smooth, so it turns
  gently on the brackets with ease in, ease out".
- `ce/src/scene/scene_view.cc` `updateCameraTurn` moves `gTurnAt` toward
  `gTurnTo` at the constant `kTurnSpeed`. Replace that with an eased motion
  from where the camera is to the target, in about the same time per step.
- A press while turning retargets without a jerk: the speed carries over
  and the camera eases into the new target (two quick presses give one
  smooth 20-degree turn). Held keys and the limit at ±30 keep working; the
  edge scroll uses the angle shown (`cameraTurnAngle`), as now.
- Zoom has its own easing (042); reuse its curve or helper if it fits.

## Acceptance

- A unit test of the eased step: starts and ends at zero speed, reaches
  the target exactly, a retarget mid-turn keeps the speed continuous.
- A short capture or a frame strip of one step and of a double press in
  `runs/150-smooth-camera-turn/`.
- ctest passes.

## Log

- 2026-11-01 Cut from the owner's request.
