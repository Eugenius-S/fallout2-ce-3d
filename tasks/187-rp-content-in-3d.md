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

