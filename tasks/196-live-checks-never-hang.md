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

### 2026-11-04 gpt

Implemented watchdog step and whole-run limits in `tools/dev-snapshot.ps1` and
an in-game watchdog heartbeat/capture request in devtools. On expiry the
runner records the last step, game state and most recent available game
snapshot, then terminates only its own test process and returns nonzero.
Added opt-in `-SkipScriptBlocking` for map-script dialogue/movie entry. The
dialogue bypass is gated by the explicit test flag.

Validation: configured and built successfully with MSVC 14.40 / Windows SDK
10.0.22621; CTest 20/20 passed; PowerShell parser and `git diff --check`
passed. Runtime evidence is in `runs/196-live-checks/`; all 46 JPEGs are under
1 MB, and JSON sidecars point to the included JPEGs.

The 165 walk case was exercised three times per map (V15SENT, VAULT15, NCR3).
All nine were bounded by the step watchdog. V15SENT and VAULT15 record the
map-script dialog state; NCR3 records a 20-step path and then no further
progress. Screenshots are the last available frame before timeout, not a
frame captured at the exact expiry instant.

The 156 resolution-change sequence completed 11/11 steps in three runs using
a fresh build of the PR source; each logged `DEV_INPUT PASS script complete`
and `settings-saved` (~17 ms). Logs and frames are in `task156-a3`, `-a4`,
and `-a5`.

Task 127 had two valid corrected-config live captures (`task127-a3` and
`task127-b4`, `fps_counter=1`, 3D scene active). Both lack the requested FPS
and frame-time text. They are consecutive failed attempts; no more 127
runtime attempts will be made. The frames are byte-identical. Likely cause:
the fly overlay is composited with `scene_renderer::drawOverlay`, while the
devtools live snapshot reads the indexed framebuffer before that GL overlay.
This needs a capture path that reads the presented GL frame; the 127 feature
remains unverified.

Tasks 140 and 146 have not been re-run under this watchdog build. Thus the
five-case/three-run acceptance matrix is incomplete; keep PR #270 draft and
task status `doing` until those cases are either verified or reach their own
attempt limits. No code from those task branches was changed.
