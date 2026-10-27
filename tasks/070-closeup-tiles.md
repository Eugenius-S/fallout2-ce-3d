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

### 2026-10-27 gpt: close-up filtering in progress

History search with `git log -S` found nearest filtering in floor and scenery
since the original renderer, and no mipmap, GL_LINEAR or upscale path for
these scene textures. The prior drum work changed geometry, not filtering;
there is no disabled close-up texture option in the current config.

The native object in the reference is at KLADWTWN elevation 0, tile 26106.
The screenshot pose was reconstructed with an isolated fly-camera lift; the
same scene and pose are compared in runs/070-closeup-tiles/closeup.jpg.
Strongly magnified fly-view art now uses contrast-aware RGB interpolation,
while nearest alpha preserves transparency, picking and material tags. The
game view remains nearest-neighbour. No shadow code was changed.

Release build, 18 CTests, 64 exporter and 12 regression unit tests passed.
All 16 game-view captures matched the references with zero changed pixels on
the first build. Final regression and fly tour are running after narrowing the
filter to strongly magnified art; confirm build hashes, rerun any earlier
captures from the previous build, and inspect the tour before finishing.

### 2026-10-27 gpt: claimed from main with 092

Worktree E:/Projects/fallout2-ce-3d-gpt-070 starts at main 4c23fc1c,
which contains 092. Clean Release build and 18 CTests passed. Test games
use isolated run folders; collect evidence through dev-snapshot.ps1
-OutputDirectory. The owner asked to keep all shadow code untouched.
Next: inspect filtering, mip and upscale history with git log -S, reproduce
the reference close-up, fix the cause, and run the original-view regression
and fly tour before the PR.
