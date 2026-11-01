---
id: 134
title: The save preview shows the 3D view cleanly
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
