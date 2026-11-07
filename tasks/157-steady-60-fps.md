---
id: 157
title: A steady 60 FPS: no uneven frames in the game view or in flight
milestone: M1
status: todo
owner: claude
suggested: claude
priority: high
tier: 2
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

## Log

- 2026-11-07 (main 990d7756, warm scene cache, off-screen 1920x1080, shadows
  and glow on): kladwtwn game camera 46.2 fps average (21.6 ms, 1% low 40.0,
  worst 34 ms); kladwtwn fly 60.7 fps, riding the 60 fps limiter; newr1 game
  camera 28.5 fps (35.0 ms, 1% low 15.6 fps, worst 66 ms). The owner raised
  the bar the same morning: below 60 is not acceptable, below 120 is not
  comfortable; the target of this task moves to a steady 120+ on the owner's
  144 Hz screen.
- F2CE_PERF pass breakdown (shadows and glow on): the floor pass is the top
  GPU cost, 7.3-10.0 ms on kladwtwn and 12.5 ms on newr1, about half the GPU
  frame; walls 2.3-3.4 ms, flat objects 1.5-1.7 ms, objects 1.2-1.9 ms,
  hero egg glass up to 2.8 ms on newr1, GPU frame 14-22 ms, CPU submit
  2.5-5.3 ms. On newr1 the frame averages 55 ms against 22 ms of GPU: about
  33 ms per frame of stalls outside the GPU (the 155 finding holds). One
  883 ms single-frame spike on kladwtwn, the same stall class as task 206.
- First change: the game view paces to a new [3d] fps_limit setting (60
  default, 0 removes the limit) with a sleep-plus-spin throttle, so the limiter
  stops hiding the render headroom behind the classic 60.
- 2026-11-07, owner order: the 60 cap must go (more is welcome), and until
  every location holds a steady 60+ the next phases do not start. This task
  is the gate: the floor-pass GPU cost and the off-GPU stalls above are the
  remaining work.
