---
id: 196
title: The background live checks never hang, a stuck run ends with its reason
milestone: M1
status: done
owner: gpt
suggested: any
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: yes
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

The five-case/three-run runtime matrix is incomplete. Per the owner's
exception, the watchdog code may merge with those unverified runtime rows
clearly recorded here once final CI is green. No code from the other task
branches was changed.

### Watchdog investigation for Task 140

One `-DialogCursorCheck` run hit the 30-second step limit with last step
`DIALOG_FIXTURE START`, but had no screenshot: the engine was already blocked
before it could service the watchdog capture request. Added a baseline
snapshot before the fixture enters the dialog. A second run saved that
non-black 3D baseline and reached `DIALOG_CURSOR_CHECK PASS`; the wrapper then
returned nonzero because this worktree lacks `tools/check_png_pixel.py` and
its cursor pixel assertion failed. `dialogue140-baseline.jpg` and its JSON
sidecar are in `runs/196-live-checks/dialogue140-watchdog-2/`. It shows the
NPC message, not the owner's hovered reply, so it is not acceptance evidence
for Task 140. The timeout path still needs a follow-up test with the new
baseline snapshot available; no further Task 140 runs were made after the
two-attempt limit.

The Task 146 combat flag is absent from this branch's runner and devtools.
Its existing PR had already reached two failed combat starts. No additional
combat attempt was made.

### 2026-11-04 gpt: code complete

Done: Added bounded step/whole-run watchdogs, state/step diagnostics, capture
fallback, test-only script dialogue/movie skipping, and dialog-fixture
baseline snapshots. Runtime rows 165 (3x each V15SENT, VAULT15, NCR3) and 156
(3 successful transaction runs) are verified. Task 127 has two corrected
live-frame captures without FPS/frame-time text; Task 140 has two watchdog
diagnostics but still no hovered-answer acceptance; Task 146 remains
unverified in this branch because its combat fixture is absent and its prior
two starts failed. The owner permits merging this code with these rows
unverified after green CI.
Check: Release build via `tools/configure-build.ps1`; CTest 20/20; CI run
41813269064 green on the preceding implementation SHA. Latest evidence is in
`runs/196-live-checks/`.
