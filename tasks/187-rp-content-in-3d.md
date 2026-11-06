---
id: 187
title: RP's new and changed maps build in 3D under the same rules
milestone: M1
status: doing
owner: gpt
suggested: any
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: [184]
reviewed: no
---

## Goal

RP's new and changed maps and art export and show in 3D like the vanilla
ones: 3D never off, a plane or billboard for what cannot be built (173).

## Context

- The Fallout 2 Restoration Project (RP) is never committed to the
  repository, not even in `runs/`. Agents never touch the owner's `game/`
  folder or `game/mod-cache`; installing RP into `game/` is the owner's own
  step, from 184's guide.
- RP leans hard on sfall: its scripts use many sfall opcodes and hooks and it
  reads its own `ddraw.ini` settings. CE implements only part of sfall
  (`ce/src/sfall_*.cc`), so most of the work is the missing pieces.
- The exporter reads the scratch copy's DATs and mod folders in CE's own
  order; RP's art overrides vanilla art of the same name.
- Export into your own cache, never `game/mod-cache`.

## Acceptance

- Every RP map exports without errors; RP's restored areas get fly tour
  spots, kept in a separate spots file that the tour runs only when an RP
  game root is given (RP is not in CI).
- A walk through two restored areas stays in 3D.

## Out of scope

sfall gaps (186).

## Log

### 2026-11-05 gpt
Done: Exported all 19 restored-map names from the installed RPU copy into
`build/scene-cache` (36 map/elevation scenes); the export `--check` passed.
Fixed export failures for engine PID -1 script-helper records and lights at
the reset-floor intensity; scene-export tests pass 127/127. A live walk probe
on Abbey passed from tile 22703 to 18702 in 518 frames with `scene-gl` and
`scene_view=full`. Evidence: `runs/187-rp-content-in-3d/`.
Next: Verify a second restored area in 3D, then add RP-only tour spots gated
by an explicit RP game root. PR #289 stays draft.
Check: `export.py --check`; scene-export unit tests; Abbey walk log at
`build/rp-live/abbey/process.stdout.log` and sidecar.

### 2026-11-05 gpt
Done: Verified a second restored area on San Fran Dr. Sheng. The isolated RPU
scratch copy reached tile 16100 from 20100 in 542 frames; the engine reported
`scene-gl`, full scene view, and no view fallback during the walk. Evidence is
in `runs/187-rp-content-in-3d/`.
Next: Add RP-only tour spots behind an explicit `--rp-game-root` option, then
smoke-test the gated tour and run the task checks.
Check: `build/rp-live/sfsheng/process.stdout.log` and sidecar; runtime frame
`runs/187-rp-content-in-3d/sfsheng-walk.jpg`. The RPU map-cache check found all
19 maps; post-rebase scene-export tests passed 122/122.

### 2026-11-05 gpt
Done: Added 36 RP map/elevation center spots in `tools/tour/rp_spots.json`.
`tools/tour/tour.py` selects them only when `--rp-game-root` names an install
with `mods/rpu.dat`; without that option, an RP-only filter returns no spots.
Check: The no-root gate behaved as expected. Two explicit-root smoke runs
timed out before the first tour progress; the first used a stale executable,
and rebuilding before the second did not change the result. Diagnostics:
`build/test-run/runs/8f670f030a88/watchdog.txt` and
`build/test-run/runs/e8ea6a3d43bc/watchdog.txt`. The user's game and cache were
not involved.
Next: Keep PR #289 draft and investigate why the tour command stalls at the
static preview, before any further runtime attempt.

### 2026-11-05 gpt: isolate the RP tour startup stall
The two explicit-root tour runs stop at the same point: the `SFSHENG.MAP`
static preview starts while `gameState=5` (`DialogRequested`), and the scene
policy blocks presentation. The successful live San Fran walk starts in the
same state but advances to `scene_view=full`; the tour's `-Still` startup
freezes the map-enter script before the request can settle. RP tours now omit
still mode and the automatic static hero preview, then let the map enter
normally. The stamp records that startup mode so older captures cannot be
reused.
Next: Run one explicit-root SFSHENG tour after this change. If it still
times out, log the new result and leave the PR draft.
Check: prior diagnostics are in `build/test-run/runs/8f670f030a88/watchdog.txt`
and `build/test-run/runs/e8ea6a3d43bc/watchdog.txt`; the live walk log is in
`build/rp-live/sfsheng/process.stdout.log`.

### 2026-11-05 gpt: bounded tour retry also timed out
The candidate live-start change did not work: the one rerun with `-NoPreview`
and without `-Still` timed out before the map cache or scene appeared.
Diagnostic: `build/test-run/runs/19efb76b0b72/watchdog.txt`. The two earlier
runs reached the static preview but remained blocked by `gameState=5`.
The candidate `tour.py` mode was reverted; it is not validated.
Next: add a bounded fixture that lets the map-enter script finish its pending
dialog request before taking a still tour. Do not rerun the same tour path
without that code change.
Check: CTest 20/20; exporter Python tests 127 passed; regression Python tests
12 passed. Existing two restored-area walk probes remain in the evidence.
Blocked: the RP-specific tour acceptance is not yet proven. PR #289 stays draft.

### 2026-11-06 gpt: refreshed exports and isolated RP tour startup
Done: merged current `origin/main` (`b2f792de`) and rebuilt Release. Re-exported
all 36 RP map/elevation scenes from the isolated RPU 2.4.34 scratch copy into
`build/scene-cache`; every export succeeded. Changed RP tour startup to use
`-NoPreview -SkipScriptBlocking` without `-Still`, which lets map-enter scripts
advance past blocking dialogs or movies. A bounded `sfsheng` smoke produced
three turned views for e0 and e1 on the before side and for e0 on the after
side. The after-side e1 run still produced no snapshot within 132 seconds.
The tour is incomplete and is not presented as accepted. The two earlier
3D walk probes on Abbey and San Fran Dr. Sheng remain valid. PR #289 stays
draft.
Next: determine why SFSHENG elevation 1 fails to start on the after side, then
rerun only after changing that cause. Do not claim the RP tour complete yet.
Check: `build/rp-export-refresh-20261106.log`, `build/tour/before/sfsheng-e0/`,
`build/tour/before/sfsheng-e1/`, `build/tour/after/sfsheng-e0/`, and
`build/test-run/runs/98a736374b6f/`; Release build, CTest 20/20 and exporter
tests 137/137 passed.
