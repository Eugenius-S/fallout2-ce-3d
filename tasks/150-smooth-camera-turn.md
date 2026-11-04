---
id: 150
title: The camera turn step eases in and out
milestone: M1
status: done
owner: glm
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
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

### 2026-11-02 glm
Done: the eased turn. `camera_turn.h` gains `cameraTurnEaseStep`: the speed
ramps in from zero at kCameraTurnAccel (12960 deg/s^2), peaks at the old
constant speed (360 deg/s), and the braking arc lands exactly on the target;
one step is a zero-to-zero ride of about 56 ms, still under the held key's
80 ms repeat. A press mid-turn keeps the signed speed and bends into the new
target (two quick presses = one smooth 20-degree arc; a reversing press sheds
the old speed through zero). `updateCameraTurn` drives it; `kTurnSpeed` and
the constant-rate move are gone. Unit test `camera_turn_tests` pins the curve
(zero-speed ends, exact landing, no overshoot or backward move, retarget
continuity, held-key extension, the -30..30 clamp untouched); 19/19 ctest,
97 + 12 python tests, export and 16/16 regression spots at 0 changed pixels.
Capture tooling along the way: scripted input gains `key <scancode>` and
`waitms` plus a scene mode (`--dev-scene-input=`, dev-snapshot.ps1
`-SceneInput -TurnKeys`) pumped by the main loop itself: the old `--dev-input`
opens the options screen, whose loop ate every key. The sidecar JSON records
`camera_turn`; `--dev-camera-turn-keys` lets a background run keep the
rotation its isolated config set. Evidence in `runs/150-smooth-camera-turn/`:
per-frame angle and speed logs of one step and of a double press (ease in,
peak, ease out, carried speed), three stills at 0/10/20 degrees.
Next: the owner's visual check (user-check), then the merge.
Check: `ctest --test-dir build -R camera_turn`; the frame strips and stills
in `runs/150-smooth-camera-turn/`.
