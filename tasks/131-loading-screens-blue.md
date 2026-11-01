---
id: 131
title: The wide loading screens show their own colours, not blue
milestone: M1
status: todo
owner: none
suggested: gpt
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Every loading screen (7 English, 7 Russian) shows the owner's artwork in its
own colours at 16:9 and 21:9.

## Context

- The owner, 2026-11-01, live on main `e434c327`: the loading screen was
  blue. A colour or palette conversion problem, like the earlier white
  speckle palette-index error that the corrected images fixed.
- Installed now as loose overrides in `game/data/art/splash/` and
  `game/data/art/russian/splash/`: the 1147x480 RIX3 files from
  `game/user-assets/fullscreen-20261028/3440x1440-originals/rix3-install-package/`
  (FFmpeg quantized to 254 colours, written with a 6-bit palette). The
  earlier 1120x480 RIX files are kept in
  `game/user-assets/fullscreen-20261028/backup-1120-rix/`.
- Find whether the package files are wrong (channel order, 6-bit vs 8-bit
  palette values, palette slots the game reserves) or the game's splash path
  draws them wrong (the cover sizing of task 118, a palette that a later
  fade or the 3D build changes). Compare against a source PNG pixel by pixel.
- Fix the converter or the game, whichever is wrong. Write the corrected
  install package next to the old one; never commit the art.

## Acceptance

- A loading screen capture beside its source PNG for one English and one
  Russian screen at 16:9 and 21:9, in `runs/131-loading-screens-blue/`;
  colours match within the 254-colour quantization.
- Installation steps in the Log so Claude can install the corrected files
  into the owner's game.
- ctest passes; the original view regression stays pixel for pixel.

## Out of scope

The framing and crop of the loading screens (118), the menu (132).
