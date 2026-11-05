---
id: 195
title: The camera turn starts and stops softly, a held key turns in one smooth sweep
milestone: M1
status: done
owner: gpt
suggested: any
priority: high
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

A tap of `[` or `]` turns the camera its 10 degrees with a soft start and a
soft stop. A held key turns in one continuous smooth sweep with no
re-acceleration per step. A press mid-turn bends smoothly into the new
target. The wheel zoom (042) gets the same feel check.

## Context

- The owner, 2026-11-04, after playing the current main build: the easing
  is harsh; it should start smoothly and slow down smoothly.
- Follow-up to 150 (#233). Stands alone: no open camera task fits. 150's
  Log: `cameraTurnEaseStep` in `camera_turn.h` makes one 10-degree step a
  zero-to-zero ride of about 56 ms, under the held key's 80 ms repeat, so
  the motion reads as a jerk.
- Likely fix: a longer ride per step (try 200 to 350 ms) on a smooth curve
  (smootherstep or ease-in-out cubic); a held key chains the steps into one
  sweep at steady speed; a press mid-turn keeps the current speed and bends
  into the new target. The -30..30 limit stays.
- The input replay from 150 (`waitms`, `--dev-scene-input=`, dev-snapshot.ps1)
  can drive a tap and a held key for the plots.

## Acceptance

- A plot of angle against time, frame by frame, for a tap and for a held
  key, in `runs/195-camera-turn-feel/`: no jump in angular speed anywhere.
- The same plot for one wheel zoom step (042), fixed the same way if it
  shows a jump.
- `camera_turn_tests` updated to the new curve; ctest passes.
- The owner's feel check after merge.

## Out of scope

The turn limit and step size (144).

## Log

### 2026-11-05 gpt
Done: the 10-degree camera turn now takes about 0.26 s from rest to rest,
ramps speed at 640 degrees/s squared, and caps a held sweep at 110 degrees/s.
Substep integration and acceleration-limited speed changes keep the held key
from restarting each 10-degree repeat. The wheel zoom now uses a 0.2 s
Hermite glide that starts at zero speed and carries its current speed across
a mid-glide wheel event.
Evidence: `runs/195-camera-turn-feel/camera-motion-plots.jpg` plots the tap,
an eight-step hold, and one zoom step at 144 Hz; the CSV files contain every
frame sample and `metrics.json` records the measured values. The held sweep
stays at 110 degrees/s until release. The old zoom curve began at 3.62 zoom
units/s in its first frame; the new curve begins at 0.13.
Next: the owner checks the feel after merge.
Check: Release build; `ctest --test-dir build --output-on-failure` (20/20);
scene export unit tests (125); regression unit tests (12).
