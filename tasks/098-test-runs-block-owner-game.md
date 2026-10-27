---
id: 098
title: A background test run no longer stops the owner's game from starting
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

While an agent's background test game is running, the owner can still start
their own game.

## Context

- The owner hit it twice on 2026-10-27: the game closed at start, and
  `debug.log` said the program was already running. A test run from
  `tools/dev-snapshot.ps1` was open at that moment.
- `ce/src/win32.cc` skips the GNW95MUTEX for a test run
  (`F2CE_DEV_BACKGROUND`), and `ce/src/autorun.cc` skips the autorun mutex.
- `ce/src/window_manager.cc:1412-1418` still creates the title mutex in a
  test run; only the "already running" exit is skipped there. So the test
  run holds the mutex, and the owner's normal game, started after it, finds
  it and quits.

## Acceptance

- A test run does not take the title mutex: the owner's game started while a
  test run is open comes up, and a second normal game still refuses to start
  as before.
- Both checks in `runs/098-test-runs-block-owner-game/` (the two `debug.log`
  lines or a short log), never by starting or closing the owner's own
  `game/fallout2-ce.exe`: use a copy in a separate folder.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

Anything else about how test runs are launched.

## Log

### 2026-10-27 gpt
Done: Background dev-snapshot no longer acquires the FALLOUT II title mutex; the title-set sentinel remains checked, mutex cleanup is guarded, and a normal copy can launch beside the background test while a second normal copy exits with code 1. Isolated acceptance evidence is in `runs/098-test-runs-block-owner-game/acceptance-mutex-results.txt`; direct 16-view RGBA comparison and build SHA records are in `runs/098-test-runs-block-owner-game/regression-comparison.txt`, with raw captures under `build/098-regression-baseline/` and `build/098-regression-final/`.
Review: The change and isolated acceptance evidence were reviewed; the PR is ready for review.
Check: CTest 18/18 and regression unit tests 12/12 pass. Direct baseline/final RGBA is exact for 16/16 views. The repository-reference run has the same four deviations on base and final (132, 44, 5106, 492039 pixels); its other 12 views pass.
