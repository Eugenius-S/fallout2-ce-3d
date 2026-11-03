---
id: 155
title: The game runs smooth: no hitch on load or on a settings toggle, higher FPS
milestone: M1
status: done
owner: claude
suggested: claude
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

The 3D game feels as quick as a 1998 game should on the owner's machine:
- loading a map or a save has no long freeze beyond the original game's;
- switching a 3D setting (sun shadows, glow, clouds) applies without a
  visible stall;
- the frame rate in Klamath downtown with all default 3D options on stays
  well above today's, with the target set from the measurements.

## Context

- The owner, 2026-11-02: hitches on load, a stall when shadows are turned
  on, low FPS overall; "a 1998 game, even in 3D, should not run like this".
- Measure first: frame time split by pass (scene build, shadow map, glow,
  2D composite, present), the load path (scene export, cache reads, texture
  uploads, shader compiles) and what a setting toggle rebuilds. The FPS
  counter (127, #181) may help; do not wait for it.
- Likely suspects, unconfirmed: shaders or render targets built on the
  toggle instead of at start, scene geometry rebuilt on a toggle, CPU-side
  work per frame that could be cached, a synchronous cache read on load.

## Acceptance

- A short measurement table before and after (Klamath downtown, owner's
  default settings, the owner's resolution): average and 1% low frame time,
  map load time, shadow toggle stall time, in `runs/155-performance/`.
- Each fixed cause named in the Log with its saving; the original view
  regression stays pixel for pixel; ctest passes.

## Out of scope

Changing resolution, window mode or scale without restart (156). Lower
visual quality as a default.

## Log

- 2026-11-02 Cut from the owner's request.

### 2026-11-02 claude
Done: GPU time per frame in Klamath downtown at the owner's settings fell from
20.6 ms to 11.8 ms (average 48 to 56 fps, the cap is 60). Causes and savings:
the 8192 sun shadow map was redrawn and gap-filled every frame, now cached
until a caster changes or the sun turns 0.02 degrees (4.2 ms); every ground,
flat object and object pixel walked 25 shadow taps even far from any caster,
now skipped through an occupancy map of the shadow map, exactly the same
picture (4.3 ms); the 770 MB of shadow textures were all allocated in one
frame on the first shadow toggle (60 to 190 ms), now one per frame and at scene
load (worst frame 48 to 51 ms). Map load stays about 300 ms. A dev frame
profiler (`F2CE_PERF`, docs/p1-devtools.md) was added. Table:
`runs/155-performance/results.md`. ctest, unit tests and the pixel regression
pass.
Next: ground shadows still cost 6.6 ms against 0.9 ms with shadows off; a
cheaper tap loop for the ground is the next saving. The 1% low frame is set by
game loop stalls outside the renderer.
Check: `F2CE_PERF=1 F2CE_PERF_HOLD=12` with `tools/dev-snapshot.ps1 -Map
kladwtwn -Shadows -Glow -ScreenWidth 2310 -ScreenHeight 1298`.
