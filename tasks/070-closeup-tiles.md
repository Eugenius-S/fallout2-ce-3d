---
id: 070
title: Tiles up close in the fly camera stop looking like pixel noise
milestone: M1
status: todo
owner: none
suggested: glm
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Up close in the fly camera the floor tiles, the walls and the things read as
clean pictures again, not raw pixel noise.

## Context

- The owner's find of 2026-10-22: `runs/inbox/2026-10-22-fly-closeup-tiles.png`
  (a round tank with its cap, the carpet floor, the wall beside it). They
  remember these were made nearly clean earlier.
- First find out whether an earlier fix (texture filtering, mips, upscaled
  tiles or shapes) was lost, is off in the owner's config, or never covered
  this case; `git log -S` on the filtering calls is the quick way.

## Acceptance

- The same close-up before and after under `runs/070-closeup-tiles/`.
- The game's own view stays pixel for pixel (the regression).

## Out of scope

New art.

## Log
