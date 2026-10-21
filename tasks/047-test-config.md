---
id: 047
title: Test runs keep their own config, apart from the owner's game
milestone: M0
status: done
owner: glm
suggested: glm
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

Test runs never read or write `game/fallout2.cfg`, so the owner's settings
(zoom, rotation, sound) never change a test result and a test never changes
the owner's settings.

## Context

- The game reads `fallout2.cfg` next to its exe (`ce/src/game_config.cc`).
  The test exe now runs as `game/fallout2-ce-test.exe` and so shares the
  owner's config. PR #46 broke the regression this way; #47 patched it by
  forcing zoom and rotation in background runs (`applyFixedScreenOptions`,
  `ce/src/devtools/devtools.cc`).
- `tools/dev-snapshot.ps1 -ConfigDirectory` already runs the test exe from an
  isolated `build/task009-*` folder with its own `fallout2.cfg` and
  `ddraw.ini` while the resources stay in `game/`. Make that the default for
  every test run (snapshots, the regression, the tour, the shapes eval): the
  script builds the folder from a fixed config kept in `tools/`, not from the
  owner's file.
- Sound off in that config, so `SDL_AUDIODRIVER=dummy` stays only as a spare.

## Acceptance

- A test run leaves `game/fallout2.cfg` byte for byte unchanged.
- Setting a zoom of 3 and `camera_rotation=1` in `game/fallout2.cfg` changes
  no regression view.
- The regression and `eval.py shoot --only eggs7.frm` pass.

## Log

### 2026-10-21 glm
Done: every test run now isolates its config. `tools/dev-snapshot.ps1` always
copies the build as `build/test-run/fallout2-ce-test.exe` with a fresh
`fallout2.cfg` from the new `tools/test-config/` (the owner's environment it
was shot from, sound `initialize=0`, `zoom=1.000000`, `camera_rotation=0`,
`[screen]` pinned to the regression values); the game keeps reading and saving
its config next to the exe (`ce/src/game_config.cc`), so `game/fallout2.cfg`
is never read or written. `-ConfigDirectory` and its `task009-*` rule are
gone; the regression, the tour and the shapes eval go through dev-snapshot and
need no changes. `applyFixedScreenOptions` zoom and rotation forcing and
`SDL_AUDIODRIVER=dummy` stay as spares. Docs updated (regression README,
p1-devtools.md). The exe never reads `ddraw.ini` (no reference in `ce/src`),
so the fixed config folder holds none.
Next: nothing.
Check: a snapshot run left `game/fallout2.cfg` byte for byte unchanged
(SHA-256 equal). With `zoom=3` and `camera_rotation=1` written into the
owner's config, the full regression passed (exit 0; 15 spots 0 changed
pixels, klatoxcv-e1 the same 4 tolerated pixels as with a clean config),
then the config was restored byte for byte. `eval.py shoot --only eggs7.frm`
passed (kladwtwn 0, 5 views). ctest: 11/11.
