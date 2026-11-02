---
id: 156
title: Resolution, window mode and scale change on the fly, without a restart
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: high
claude-only: no
user-check: yes
depends-on: [154]
reviewed: no
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
- The switch takes under a second; the original view regression stays
  pixel for pixel; ctest passes.

## Out of scope

Speed of the game in general (155).

## Log

- 2026-11-02 Cut from the owner's request; 154 asked for a restart.

### 2026-11-02 gpt: implementation path

Apply display changes only after the preferences window is destroyed, then rebuild the map and interface windows for the new logical dimensions without unloading the map or objects. `svga.cc` owns the logical SDL surfaces and presentation, the window manager owns a screen-sized backing buffer and root rectangle, `map.cc` owns the iso window and tile/object buffer pointers, and `interfaceInit` lays out the bottom bar. The GL presenter holds logical dimensions while render targets adapt to drawable size; mouse input caches the window-to-logical mapping. Open dialogs and menus must not be left with old-sized buffers: defer the transaction until the preferences window and any enclosing modal screen are closed. For window mode use SDL flags on the existing window. Preallocate the new surfaces and window buffers, validate the mode, then swap and rebind these pieces; if any stage fails restore the previous window mode, dimensions, surfaces, settings and pointers. Measure the complete operation and verify actual rendered frames and cursor mapping before declaring success.
