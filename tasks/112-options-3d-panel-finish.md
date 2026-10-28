---
id: 112
title: The options screen's 3D panel: buttons under it, a straight scrollbar, a clean side frame, shadow density 0.8
milestone: M1
status: todo
owner: none
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The options screen with the 3D panel beside the game's own preferences
(017) looks finished: the game's three bottom buttons sit under the 3D
panel, its scrollbar is straight and clean, its side frame is a plain
repeat of the corner art, and the shadow density starts at 0.8.

## Context

The owner, 2026-10-28, live on main `9705aca4`:

- `runs/inbox/2026-10-28-options-buttons.png`: the game's buttons
  (default, done, cancel) stay at the bottom of the original preferences
  page. The owner: move them down under our extra screen (the 3D panel).
- `runs/inbox/2026-10-28-options-3d-scrollbar.png`: the 3D panel's
  scrollbar looks crooked. Its groove is built in
  `OptionsPanel` (`ce/src/scene/options_panel.cc`, `scrollbarGroove_`) by
  copying a turned strip of the preferences background, and the knob is
  `PreferencesSliderOff`.
- `runs/inbox/2026-10-28-options-3d-frame-side.png`: the 3D panel's
  side frame (from the `PipBoyWindow` art). The owner: make the side a
  plain repeat of one pixel column taken from the corner piece, so the
  edge runs straight from corner to corner.
- The shadow density default goes from 1.0 to 0.8
  (`settings.h` `shadow_density`, and the value `settings.cc` writes when
  it adds the key). An ini that already holds the key keeps its value: do
  not rewrite players' files; the owner sets theirs in the panel.

## Acceptance

- before | after of the whole options screen and of the scrollbar and the
  side frame up close, at 16:9 and at 4:3, in
  `runs/112-options-3d-panel-finish/`; the buttons work where they now
  stand (mouse and keys), and the original 4:3 screen without the 3D panel
  keeps its buttons where the game has them.
- A new ini gets `shadow_density=0.8`; ctest (settings_3d, the options
  panel model tests) passes; the original view regression stays pixel for
  pixel.

## Out of scope

New settings, the panel's text and layout of rows, the game's own
preferences page art.

## Log
