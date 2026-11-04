---
id: 132
title: The main menu's lower left shows the art clean, no blocky band
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The owner's wide main menu art shows clean across the whole screen: no
blocky, posterized or off-colour band anywhere, in particular in the lower
left under the buttons.

## Context

- The owner, 2026-11-01, live on main `e434c327`, Russian menu at 16:9:
  `runs/inbox/2026-11-01-menu-bottom-left-blocks.webp` (the red arrow). The
  lower left shows a band of coarse rectangular blocks in darker, flatter
  blue than the art around it.
- The menu loads the RGBA wallpaper
  `user-assets/fullscreen-20261028/wallpapers-selected/hires/menu/art__intrface__hr_mainmenu__d0_f0_21-9.png`
  (task 118, `ce/src/mainmenu.cc`) with palette-drawn controls above it.
  Suspects: the legacy indexed menu background or a palette-drawn rectangle
  painted over the wallpaper there, the copyright line's backing, or the
  wallpaper file itself. Check the source PNG at that spot first.
- The Russian labels overlapping the red buttons are task 126, not this.

## Acceptance

- before | after of the whole menu at 16:9 and 21:9, plus a crop of the
  lower left beside the same crop of the source PNG, in
  `runs/132-menu-bottom-left-blocks/`.
- Every menu button still works by a mouse click.
- ctest passes; the original view regression stays pixel for pixel.

## Out of scope

The menu art itself, its framing (118), the labels (126).


## Log

### 2026-11-01 gpt
Done: confirmed the supplied wallpaper PNG is clean. The defect comes from a broad `mainMenuMaskBackground` rectangle that leaves a large area to the indexed framebuffer. Removed that mask; button, panel, label, and footer masks remain. Added the owner's before screenshot and a mapped lower-left crop beside the matching source-art crop under `runs/132-menu-bottom-left-blocks/`.
Check: Release build passed; `ctest --test-dir build --output-on-failure` passed 18/18. `tools/regression.ps1` passed all 16 spots against `origin/main` references with 0 changed pixels, using `build/scene-cache` and the isolated `build/task132-game` copy. Two attempts to capture the live menu in that copy returned a white frame through `PrintWindow`, including a no-activate window capture; the owner's game was not started, closed, focused, or modified.
Next: Claude should capture the patched menu in-game at 16:9 and 21:9, verify the menu buttons by mouse click, and add those before/after captures before marking this task done. This source-only fix has no art or data payload to install; do not copy files into `game/data` or alter the existing RIX3 package.

- 2026-11-01 Closed at merge (#190); the owner checks it live.
