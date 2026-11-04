---
id: 195
title: The camera turn starts and stops softly, a held key turns in one smooth sweep
milestone: M1
status: todo
owner: any
suggested: any
priority: high
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: no
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
