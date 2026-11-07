---
id: 134
title: The save preview shows the 3D view cleanly
milestone: M1
status: done
owner: gpt
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
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

### 2026-11-05 gpt follow-up
Done: Found a non-empty pre-existing save in the archived `runtime-review-20261018` test profile. The stored preview bytes contain 29,495 non-zero values. A read-only copy was placed under the isolated build scratch root; the background existing-save fixture passed and displayed the old image and label (Narg, 25 JUL 2241, 0824, Arroyo Temple Entrance). The original source save was not modified. Added the full live load-screen capture, a detail crop, and JSON metadata. Preserved the paired 3D/classic save-load sheet and added both sidecars to the PR evidence.
Next: Rebuild and run CTest plus the original-view regression on the branch after its merge with current origin/main. If clean, set task status done and wait for the final SHA's green CI before marking PR ready.
Check: `tools/dev-snapshot.ps1 -ExistingSavePreviewFixture -CaptureConsole -OutputDirectory runs/134-save-preview-3d/oldsave-runtime-review -GameRoot build/134-oldsave-runtime-review-game -SceneCache build/scene-cache -TestDirectory build/test-run-134-oldsave` passed. Source SAVE.DAT preview at offset 131 is 29,792 bytes, 29,495 non-zero, 84 unique indices. Captured frame is in `runs/134-save-preview-3d/oldsave-existing-preview.jpg`; inspected detail crop shows the stored picture and matching save label.

### 2026-11-05 gpt finish
Done: The pre-existing save preview now has matched live evidence. The 3D and classic save/load sheet is preserved with sidecars, and the archived old-save fixture shows its stored Arroyo Temple thumbnail and matching label. The untouched old save loads. The owner source files were read-only; all runtime testing used an isolated copied profile. After merging current origin/main, the Release build passed, CTest passed 20/20, and the full original-view regression passed with 0 changed pixels at every capture.
Next: Await CI on the final pushed SHA; then mark PR #297 ready and squash-merge if CI is green.
Check: `cmake --build build --config Release --parallel 12` with VS 14.40; `ctest --test-dir build -C Release --output-on-failure` (20/20); `tools/regression.ps1` (all captures 0 changed pixels); existing-save live fixture log and JPEGs in `runs/134-save-preview-3d/`.
