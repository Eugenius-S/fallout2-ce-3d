---
id: 134
title: The save preview shows the 3D view cleanly
milestone: M1
status: blocked
owner: gpt
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

When the player saves in 3D, the preview picture in the save and load
screens shows the 3D view as the player saw it: the same framing round the
hero, sharp, the right colours, not stretched or cut.

## Context

- The owner, 2026-11-01, live on main `e434c327`: the preview of a save
  made in 3D looks wrong (`runs/inbox/2026-11-01-save-preview-3d.png`, the
  load screen, slot 01). The original game takes the preview from its 2D
  window buffer; in 3D that buffer is not what the player sees.
- The original view's preview must stay as it is.

## Acceptance

- before | after of the load screen with a save made in 3D, and of one made
  in the original view (unchanged), in `runs/134-save-preview-3d/`.
- Old saves still load and show their old previews.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

The save and load screens' layout, wide screen fit of those screens.

## Log

### 2026-11-05 gpt
Done: save-time 3D capture is quantized into the existing indexed thumbnail format. The 3D and original-view paths both save and reload slot 01 in the isolated test profile. Runtime evidence is in `runs/134-save-preview-3d/`; CTest passes 20/20 and Python suites pass 125 and 12 tests.
Next: repeat the old-save comparison with a pre-existing save that has a non-empty thumbnail, then capture a matched before/after from the same map and camera.
Check: `3d-final/process.stdout.log`, `classic-final/process.stdout.log`, `existing-save/process.stdout.log`, and `runs/134-save-preview-3d/current-3d-vs-classic.jpg`.
Blocked: the available existing slot 01 loads, but its stored 29,792-byte thumbnail is entirely palette index 0. The supplied screenshot shows a different save state and cannot serve as a pixel-matched baseline. The game save under the main checkout was only read, then copied into the scratch profile; the source was not changed.



