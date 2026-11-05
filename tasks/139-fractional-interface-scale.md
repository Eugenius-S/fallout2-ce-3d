---
id: 139
title: The game scale picked in the options menu in steps of 0.1x, not only whole numbers
milestone: M1
status: done
owner: none
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The player picks how big the game picture and interface are in the options
menu: 1.0x, 1.1x, 1.2x, 1.3x and so on in steps of 0.1x, up to the largest
scale that still leaves the game its 640x480 logical screen, instead of the
whole-number `scale` in fallout2.cfg (now x2 on the owner's screen).

## Context

- The owner, 2026-11-01: a scale in the menu with a 0.1x step.
- Today the scale is the integer `[screen] scale` (`settings.screen.scale`,
  `ce/src/svga.cc` around line 174): the logical framebuffer is the window
  size divided by it, and CE drops the scale when the result would be under
  640x480 (`tools/dev-launch.ps1` mirrors that rule). A fractional scale
  makes the logical size the window divided by the factor, rounded to whole
  pixels, and the final picture is stretched to the window.
- The stretch keeps pixel art clean: nearest neighbour or a sharp-bilinear
  filter, not a soft blur; the sharp text pass (025) and the 3D view draw at
  the window's real resolution, so text stays crisp at any factor.
- The row goes into the options screen next to the 3D rows
  (`ce/src/scene/options_panel.cc`, labels in `ce/src/scene/menu_text.cc` in
  English and Russian); a change takes effect without restarting the game
  if the engine allows, else on the next start with a note in the row.
- An existing whole-number `scale` in a player's ini keeps working and reads
  as that factor.

## Acceptance

- Shots of Klamath downtown and of the dialogue screen at 1.0x, 1.3x, 1.5x
  and 2.0x on a 16:9 window in `runs/139-fractional-interface-scale/`: the
  interface fits the window, clicks land where the cursor points.
- At 2.0x the picture matches today's `scale=2` pixel for pixel; the
  original view regression stays pixel for pixel; ctest passes, with a
  settings test for the new key and for reading an old integer `scale`.

## Out of scope

The 3D camera zoom, new art for any size, the font clipping (140).

## Log

- 2026-11-01 Moved to gpt so claude takes 135, 149 and 129.
- 2026-11-02 Folded into 154 (resolution, window mode and a 1.0x-3.0x scale slider in 0.2x steps, the owner's newer ask). Closed here; the work is 154.
