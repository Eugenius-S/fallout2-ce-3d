id: 119
title: Composite owner fullscreen art with the 3D interface
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: [118]
reviewed: yes
---

## Goal

Show the owner's fullscreen background art behind the existing UI in the 3D presentation path.

## Context

When `showDeath` loaded `death.png` successfully in the isolated test copy, the full-view compositor still displayed the old narrow death frame. `renderPresent` returned from its 3D full-view branch before applying the RGBA background. The same branch serves other fullscreen owner backgrounds.

## Acceptance

- The 3D full-view path composites the RGBA background and preserves masked UI such as death subtitles.
- The ordinary 2D path and main menu keep their current output.
- ctest passes and the original scene output matches `origin/main` pixel for pixel.

## Out of scope

Changing owner artwork, game settings, save files, or the owner's running game.

## Log

### 2026-10-29 gpt

Done: Fixed the full-view compositor to apply the RGBA background between palette conversion and the UI overlay, honoring the background mask. Isolated captures show the death artwork edge to edge in 3D and ordinary 2D presentation, the subtitle on top, and the main menu after fade. Evidence: `runs/118-wide-fullscreen-art-in-game/death-compositor-fix/`.

Check: Release build; `ctest --test-dir build --output-on-failure` passed 18/18. Full regression against references reported 14/16. The two failures (klagraz-e0: 183 pixels, klaratcv-e2: 6299 pixels) reproduced on clean `origin/main` with the same counts; direct branch-to-main PNG comparisons at both points were 0 different pixels. Sidecars identify CE commit 27ded3a6 and the final regression EXE SHA-256 `707ab34152471be1a725bf60ebdba553ba9e75e5a55d625ce0791906ef64045f`.

Limit: Runtime screenshots used a temporary isolated test hook to force the death scene; those hooks were removed before the final build. The owner's game, configuration, and cache were not changed.
