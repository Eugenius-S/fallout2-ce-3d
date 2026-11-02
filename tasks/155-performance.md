---
id: 155
title: The game runs smooth: no hitch on load or on a settings toggle, higher FPS
milestone: M1
status: todo
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
