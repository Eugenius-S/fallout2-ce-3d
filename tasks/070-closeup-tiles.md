---
id: 070
title: Tiles up close in the fly camera stop looking like pixel noise
milestone: M1
status: doing
owner: gpt
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

### 2026-10-27 gpt: claimed from main with 092

Worktree E:/Projects/fallout2-ce-3d-gpt-070 starts at main 4c23fc1c,
which contains 092. Clean Release build and 18 CTests passed. Test games
use isolated run folders; collect evidence through dev-snapshot.ps1
-OutputDirectory. The owner asked to keep all shadow code untouched.
Next: inspect filtering, mip and upscale history with git log -S, reproduce
the reference close-up, fix the cause, and run the original-view regression
and fly tour before the PR.
