---
id: 196
title: The background live checks never hang, a stuck run ends with its reason
milestone: M1
status: doing
owner: gpt
suggested: any
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Every background live check (walk probe, dev-snapshot with scene input, the
map tour, the combat fixture) either finishes or stops within its time
limit with a reason: the game state, the last step, a screenshot. A map
script's dialogue or a movie on entry never leaves a probe waiting forever.

## Context

- Tier 0, a blocker: on 2026-11-04 gpt stopped five tasks on it, each after
  two attempts. Stands alone: it sits under every batch, and 183 (the dev
  tools batch) needs 127, which waits on it.
  - 165 (#265, merged): the walk probes on V15SENT e0 and VAULT15 e0 wait
    forever once the map script opens a dialogue
    (`GameState::DialogFinished` 2, `GameState::DialogActive` 4, game mode
    0x4); NCR3 e0 finds a path of length 20 and then logs nothing
    (`runs/165-ncr-in-3d/walkprobe-*`).
  - 156 (#227): a resolution change passed once, then two runs hung. Find
    out whether the harness or the change hangs; a hang in the change is
    156's bug and goes back there.
  - 127 (#181): no live 3D frame with the FPS counter in two runs.
  - 140 (#206): no frame of a hovered dialogue answer in two runs.
  - 146 (#220): the combat fixture never reached its start.
- Code: `docs/p1-devtools.md` (dev-snapshot, scene input `waitms`,
  `--dev-scene-input=`), `tools/dev-snapshot.ps1`, the walk probe, the tour.

## Acceptance

- A watchdog on every live check: a step limit and a whole-run limit; on
  expiry it writes the game state, the last step and a screenshot, then
  kills the game and exits non-zero.
- A dev option closes or skips a script dialogue and a movie that opens
  while a probe runs, and logs it.
- The five cases above run three times each: each run finishes or stops
  with a stated reason inside its limit; the reasons are in the Log.
- ctest passes.

## Out of scope

The fixes those reasons point to (they go back to their tasks).

## Log
