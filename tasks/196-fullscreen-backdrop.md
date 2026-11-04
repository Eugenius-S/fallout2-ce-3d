---
id: 196
title: Full-screen screens stand on a black backdrop that fades in, with rounded corners
milestone: M1
status: todo
owner: any
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Every screen that filled 640x480 in the original stands on a black backdrop
on a wide or large window, instead of the 3D world showing around it. The
backdrop fades in when the screen opens and out when it closes, the same
everywhere, and the screen's panel gets softly rounded corners.

## Context

- The owner, 2026-11-04, with a frame from the showcase video of the
  dialogue screen centred on black
  (`runs/inbox/2026-11-04-fullscreen-black-backdrop.webp`): everywhere a
  screen was full-screen and the world now shows round it, the same black
  backdrop, darkening smoothly with a fade; the character screen, dialogue,
  the world map, the Pip-Boy in general; and round the panel's corners a
  little, by 4 to 6 pixels.
- The UI batch. Its own task: 128 is the wide HUD art and its rights, a
  different thing. Linked to 173: the world map, dialogs and menus stay 2D,
  and this backdrop is what sits round them on a local map.
- Screens: the character screen, dialogue, the world map, the Pip-Boy, and
  every other screen that filled 640x480 in the original (inventory, barter,
  loot, options, save and load, the skilldex if it is full-screen).
- The fade: about 150 to 250 ms, eased, in on open and out on close, one
  shared value for every screen.
- The corners: the panel art cut round by 4 to 6 window pixels to the
  backdrop, at any scale.

## Acceptance

- Before | after of each listed screen at 16:9 and 21:9 in
  `runs/196-fullscreen-backdrop/`.
- A frame-step capture of the fade in and the fade out.
- The corners checked at x1 and x2.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

The wide HUD bar (128), the 4:3 art extension (128 step 2).

## Log
