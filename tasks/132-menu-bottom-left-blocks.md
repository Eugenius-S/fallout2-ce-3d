---
id: 132
title: The main menu's lower left shows the art clean, no blocky band
milestone: M1
status: doing
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

