---
id: 157
title: A steady 60 FPS: no uneven frames in the game view or in flight
milestone: M1
status: todo
owner: claude
suggested: claude
priority: high
claude-only: no
user-check: yes
depends-on: [155]
reviewed: yes
---

## Goal

The game holds a steady 60 FPS on the owner's machine with the default 3D
options on: no uneven pacing, no periodic hitches, in the game camera and in
flight.

## Context

- The owner, 2026-11-02, on main `e8f168a2` after 155: it still runs
  unevenly ("как-то криво идёт"); they want a smooth 60.
- 155 left two leads (`runs/155-performance/results.md`): ground shadows
  cost 6.6 ms against 0.9 ms with shadows off; the 1% low frame is set by
  game loop stalls outside the renderer.
- Check the pacing as well as the cost: the frame limiter
  (`sharedFpsLimiter`), vsync and swap interval, CE's tick-based game loop
  against the render rate, work done every Nth frame. The profiler is
  `F2CE_PERF` (docs/p1-devtools.md); the FPS counter is 127.

## Acceptance

- A frame time trace of 30 s in Klamath downtown (game camera, walking, and
  a flight) before and after, in `runs/157-steady-60-fps/`: average at or
  under 16.7 ms, 1% low and the worst frame named, each cause of a spike
  written in the Log with its fix.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

Lower visual quality as a default; display changes without restart (156).

## Log

- 2026-11-02 Cut from the owner's find; follows 155.
