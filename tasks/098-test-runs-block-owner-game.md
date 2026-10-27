---
id: 098
title: A background test run no longer stops the owner's game from starting
milestone: M1
status: todo
owner: none
suggested: gpt
priority: normal
claude-only: no
user-check: no
depends-on: []
reviewed: no
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
