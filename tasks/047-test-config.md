---
id: 047
title: Test runs keep their own config, apart from the owner's game
milestone: M0
status: todo
owner: none
suggested: glm
claude-only: no
user-check: no
depends-on: []
reviewed: no
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
