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

### 2026-11-02 gpt: paused, build environment

PR #227 remains draft. Live display changes are not connected: the current
settings still require a restart. Reversible SDL window, surface, root buffer,
map window and tile/map/text view preparations have been committed. The dev
input runner can reopen preferences in one process. The last completed test
run before the object-table work passed 21/21 tests; the reopen diagnostic
frames only prove pending settings, not a live switch. The acceptance run,
switch timings, pixel regression on the completed change and CI remain open.

Stopped after repeated MSVC build environment failures. A plain
`cmake --build build --config Release --parallel 4` failed with
`fatal error C1083: stdio.h: No such file or directory`. Running the same
build after `VsDevCmd.bat -arch=x64` failed with `vcruntime.h: No such file
or directory`; VsDevCmd supplied the Windows SDK include paths but omitted
the MSVC include path. A further attempt to supply the MSVC path through
`cmd` lost the SDK include paths and again failed on `stdio.h` and `assert.h`.
No further build attempts were made. The unfinished, untested object-table
work was saved locally as `build/task156-object-view-unverified.patch` and
is not part of the PR. Build output is in `build/task156-object-build.log`
and `build/task156-object-build2.log`.

For continuation, the object renderer needs new viewport-dependent scratch
tables without clearing the map or player. Reposition the existing interface
bar without recreating it, preserving its buttons and state; prepare its side
panels and indicator placement as part of the same rollback-safe transaction.
Complete the modal-close integration and validate the requested one-process
runtime sequence before removing restart labels or changing this task to done.

### 2026-11-03 gpt: paused after build environment failures

Done: restored the saved object-view swap work from
`build/task156-object-view-unverified.patch`. It adds the reversible object
renderer table exchange needed by the display transaction.
Next: resume only after the Windows build environment is repaired; finish
interface-bar repositioning and modal-close integration, then run the full
single-process display-change acceptance sequence. PR #227 remains draft.
Check: `git apply --check` passed. Three consecutive local builds failed before
compiling this change: MSVC headers were initially missing from `INCLUDE`; after
adding them, UCRT `assert.h` and `stdio.h` could not be resolved. Existing CI is
green, but this implementation has no successful build or live runtime proof.
