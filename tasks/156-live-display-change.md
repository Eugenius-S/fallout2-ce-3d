---
id: 156
title: Resolution, window mode and scale change on the fly, without a restart
milestone: M1
status: todo
owner: gpt
suggested: gpt
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: [154]
reviewed: yes
---

## Goal

The display rows in the 3D settings (154) take effect when the panel
closes, without restarting the game: window mode, resolution and picture
scale. The "restart needed" notes go away for what now applies live.

## Context

- The owner, 2026-11-02: wants resolution and scale to change on the fly
  and without a stall.
- 154 saves `[screen] resolution_x`, `resolution_y`, `windowed` and the
  scale factor and asks for a restart. Window mode is an SDL window flag
  change. Resolution and scale change the logical framebuffer size
  (`ce/src/svga.cc`), so the SDL texture, the 3D render targets and the 2D
  game windows (interface bar, map view, open dialogs) laid out at start
  for that size must be rebuilt or re-laid out. Find what CE sizes at start
  and choose: rebuild live everywhere, or live from the options screen with
  the map window re-created on return. Write the choice in the Log first.
- A mode that fails falls back to the previous one, as in 154.

## Acceptance

- Scripted input (154's dev input) switches 1920x1080 1.4x window to
  2560x1440 2.0x and to borderless and back in one run, with frames after
  each switch in `runs/156-live-display-change/`: the interface fits, the
  cursor lands where it points, the 3D view fills the window.
- Every 3D settings row (shadows, glow, clouds, lights, scale, resolution)
  applies without a stall: measured with 127's frame-time log, no frame
  over 100 ms on the change. A change never re-exports or re-reads the scene
  cache; it rebuilds only the render targets or shader state it touches.
- The switch takes under a second; the original view regression stays
  pixel for pixel; ctest passes.

## Out of scope

Speed of the game in general (157).

## Log

- 2026-11-02 Cut from the owner's request; 154 asked for a restart.
