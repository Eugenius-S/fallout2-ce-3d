---
id: 154
title: Resolution, window mode and picture scale in the 3D settings
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The player sets the screen in the game, in the 3D settings' first section
("Основные" / Main), without editing fallout2.cfg:
- a dropdown with every resolution the display offers;
- a dropdown: fullscreen, window, borderless;
- below them a slider for the picture and interface scale, 1.0x to 3.0x in
  steps of 0.2x.

## Context

- The owner, 2026-11-02, on the panel
  (`runs/inbox/2026-11-02-display-options-place.png`): "add a resolution
  choice here (a dropdown with all available resolutions), a dropdown
  fullscreen / window / borderless, below a scale slider from 1x to 3x in
  0.2x steps". High priority.
- Folds in 139 (fractional scale), whose context still holds: today the
  scale is the integer `[screen] scale` (`settings.screen.scale`,
  `ce/src/svga.cc` around line 174), the logical framebuffer is the window
  divided by it and must stay at least 640x480; a fractional factor makes
  the logical size the window divided by it, rounded, and the final picture
  is stretched without blur (nearest or sharp-bilinear). The sharp text and
  the 3D view draw at the window's real resolution. A factor that would
  leave less than 640x480 is not offered (the slider stops there, or the
  step greys out with a note).
- `[screen] resolution_x`, `resolution_y` and `windowed`
  (`WindowMode`, `settings.h`) are the stored keys. The resolution list comes
  from SDL's display modes for the window's display, without duplicates,
  largest first; the current one is selected even if SDL does not list it.
  Borderless is a desktop-sized window without frame on the current display.
- The panel (`ce/src/scene/options_panel.cc`, labels in
  `ce/src/scene/menu_text.cc` in English and Russian) has sliders and
  checkboxes but no dropdown yet: add one that fits the panel's look (the
  Pip-Boy green, the existing row height), opens over the rows below and
  closes on a choice, Escape or a click outside.
- A change applies when the panel closes if the engine allows it without a
  restart; where it needs one, the row says so and the value is saved.
  A resolution or mode that fails falls back to the previous one.
- An old integer `scale` in a player's ini keeps working and reads as that
  factor.

## Acceptance

- Shots of the open dropdowns and the slider, English and Russian, and of
  Klamath at 1.0x, 1.4x, 2.0x and 3.0x on a 16:9 window, plus one
  borderless and one windowed run, in `runs/154-display-options/`: the
  interface fits, clicks land where the cursor points.
- At 2.0x the picture matches today's `scale=2` pixel for pixel; the
  original view regression stays pixel for pixel; ctest passes, with a
  settings test for the new keys and for reading an old integer `scale`.

## Out of scope

The 3D camera zoom, new art for any size, the dialogue font clipping (140).

## Log

- 2026-11-02 Cut from the owner's request; 139 folded in.

### 2026-11-02 gpt
Done: claimed task on a branch from origin/main; no implementation changes.
Next: run this task with the requested gpt-6-luna, high effort, then implement and verify all acceptance criteria.
Check: no build or runtime checks run; the current chat cannot verify or switch its model.
