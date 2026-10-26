---
id: 089
title: Black dots on the toxic slime and black main menu button text
milestone: M1
status: todo
owner: none
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The toxic slime shows no black dots, and the main menu button labels are
yellow, as in the original game. Find where the black comes from and fix
the cause, not each place.

## Context

- The owner, 2026-10-26, in the live game: the green slime is speckled with
  black pixels (`runs/inbox/2026-10-26-slime-black-dots.png`); the main
  menu button labels, yellow in the original, come out black. The owner
  thinks it is the same cause.
- Likely suspects, to check first: palette entries that turn black on their
  way into the 3D picture (the cycling colours the slime uses, unmapped or
  zeroed palette entries, as the 073 fixture showed: it restored unmapped
  `_cmap` entries as zeros); a texture cached before the palette was filled
  and never refreshed; an index the sprite alpha or the sharp text layer
  takes for transparent or black.
- Compare with Ctrl+F10 (the original view) on the same frame: if the
  original view is clean, the cause is in the 3D path.

## Acceptance

- The cause named in the Log, with the palette index (or indices) that
  went black.
- Before | after in `runs/089-black-dots-slime-menu/`: the slime close up
  (live cycling) and the main menu buttons.
- The original view regression stays pixel for pixel.

## Out of scope

The glow strength of the slime (073, 086).

## Log
