---
id: 147
title: Map exits, elevations and the world map come back in 3D
milestone: M1
status: blocked
owner: gpt
suggested: gpt
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Every way the hero leaves a Klamath map, changes elevation or returns from
the world map ends on the new map in 3D: the right scene, no stale scene
from the old map, no long 2D frame, the camera on the hero.

## Context

- 010 gives a scene for any map and elevation; nobody has walked the
  transitions themselves since the 3D view stays on more often (135).
- Ways to cover: each exit grid between the Klamath maps (kladwtwn,
  klamall, klagraz, klacanyn, klaratcv, klatoxcv, klatrap), stairs and
  ladders between elevations (klaratcv, klatoxcv), leaving to the world map
  and entering a map from it, loading a save made on another map.
- The background run can travel (`-Tour`) and load maps; add what is
  missing to drive an exit grid or an elevation change, or explain in the
  Log why a step was checked by hand.

## Acceptance

- A table in the Log: each transition, what the first 3D frame on arrival
  shows (crop in `runs/147-map-transitions-keep-3d/`), and whether a 2D
  frame or the old scene flashed.
- Every defect found fixed by a rule, or cut as its own task with its crop.
- ctest passes; the regression stays pixel for pixel.

## Log

- 2026-11-01 Cut with the 146 pool.
- 2026-11-02 Added opt-in `--dev-transition-watch` instrumentation to record
  present mode, map/elevation, iso state and hero tile across a map identity
  change. Added an opt-in exit-grid probe to the background snapshot runner.
  These are diagnostic tools; no render policy or game content was changed.
- Runtime evidence, background instance, KLAMALL exit tile 28078 to
  KLADWTWN/e0 arrival tile 25142:

  | Transition | First arrival frame and result | 2D/old-scene observation |
  | --- | --- | --- |
  | KLAMALL/e0 -> KLADWTWN/e0 | First captured 3D frame has KLADWTWN/e0, hero tile 25142; comparison: `runs/147-map-transitions-keep-3d/klamall-to-kladwtwn-first-frames.jpg` | The captured first arrival present is black loading background with HUD, followed by 3D. This alone does not establish whether any old scene flashed or whether the loading duration is acceptable. No defect conclusion. |
  | KLAGRAZ/e0 -> KLADWTWN/e0 | Not reached | Probe selected exit tile 22161, target tile 19850/e0, but projected click at (463,232) was not pickable; no transition or arrival capture. |
  | Other Klamath exits, elevation changes, world map out/in, save on another map and reload | Not checked | No runtime evidence. |

- In the measured KLAMALL -> KLADWTWN sequence, watcher recorded 167
  presents between map identities. The last 2D callback was at 23962 ms and
  the first 3D callback at 24092 ms, 130 ms apart. The 2D callback is marked
  `iso=1` / `available=iso-disabled` during map loading; do not treat the
  measured loading callbacks or this black frame as acceptance of the
  "no long 2D frame" requirement, and do not change loading fades from this
  evidence alone.
- Capture stop condition reached after two unsuccessful generic exit-probe
  attempts. The first aborted while overlay sync was off; after fixing that
  probe guard, the second failed because the projected KLAGRAZ exit was not
  pickable. Stop further runtime capture until the probe can use the normal
  movement/picking path or a verified pickable exit.
- `ctest --test-dir build --output-on-failure`: 18/18 passed. This does not
  validate transition coverage or the pixel regression requirement.
- Status remains `doing`. Next: fix and validate the exit probe, capture each
  listed route/elevation/world-map/save-load path with first-frame sequence,
  determine whether any 2D or old-scene flash is actually visible, then run
  the pixel regression and update this table. No transition defect is yet
  confirmed or fixed.

### 2026-11-03 gpt continuation

Done: confirmed the two documented generic exit-probe attempts are the current
runtime limit. The task is already claimed as `doing` by `gpt`; no third probe
was run. PR #221 remains draft with the partial KLAMALL-to-KLADWTWN sequence
and its timing, not full acceptance evidence.
Next: continue to task 140 under the owner's two-attempt exception. Resume 147
after the exit probe follows the normal movement/picking path.
Check: existing PR CI passed; the transition table records the partial result
and explicitly leaves all other routes, elevations and world-map/save-load
paths unverified.

### 2026-11-03 gpt: evidence path cleanup

Done: removed absolute local paths from the captured stdout logs under `runs/147-map-transitions-keep-3d/`; the transition captures and timing are unchanged. The previous PR check failed only on these paths. The two failed exit-probe attempts remain the runtime limit, so no new capture was made. The transition evidence is still partial and the PR remains draft.
Next: continue with task 140 as recorded above; revisit 147 only when the exit probe can use a verified pickable route.
Check: `tools/check_public_content.py origin/main` and `tools/check_runs_size.py origin/main` pass after this commit.

### 2026-11-03 merge origin/main

Merged `origin/main` (`d8cff6b0`) into this branch. Resolved the `tools/dev-snapshot.ps1` overlap by preserving transition-watch/exit-probe completion checks together with main's scripted-input completion checks. PowerShell parser and `git diff --check` pass. Runtime transition coverage remains incomplete as listed above; PR stays draft.

### 2026-11-04 gpt: merge fresh origin/main

Merged `origin/main` at `3b288fd6` with no conflicts. The transition matrix remains incomplete and its single captured arrival does not establish flash behavior or all route coverage. PR #221 stays draft; no additional runtime attempt was made.

### 2026-11-04 gpt: merge current main

Fetched and merged `origin/main` at `8e135a73867b` into the existing task
branch; no conflicts. Pushed merge commit `b51c67952f2e4271e31af54a7f5608e6a2cdbaa9`.
The two documented exit-probe attempts remain the runtime limit; no new
capture was made. PR #221 stays draft and the transition matrix remains
incomplete.

Check: PowerShell parser, public-content check, runs-size check and
`git diff --check` pass. No new runtime or pixel-regression result is claimed.


### 2026-11-04 gpt: rebased on current main and blocked

Rebased the owned task branch onto `origin/main` at `f2ef1236` as requested.
Resolved the task-header and `tools/dev-snapshot.ps1` conflicts by retaining
the transition diagnostics and the main branch's scripted-input checks. The
two failed KLAGRAZ exit-probe attempts remain the limit; no repeat was made
without a probe-path fix. Task status is `blocked`, and PR #221 remains draft:
only the KLAMALL-to-KLADWTWN arrival is captured, with all other routes,
elevations, world-map and save/load transitions unverified.
### 2026-11-04 gpt: rebase CI handoff

The branch was force-pushed with lease at `78483130` after rebasing onto
`origin/main` `f2ef1236`. At the time of this note, GitHub still reports PR
head `fac7cd9c` and its active CI is for that pre-rebase commit, while the
branch API reports `78483130`. No new transition capture was made. The PR
remains draft because acceptance is incomplete.
