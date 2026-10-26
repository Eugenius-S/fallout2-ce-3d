---
id: 058
title: Regression references no longer match the unchanged renderer
milestone: M1
status: done
owner: claude
suggested: glm
priority: high
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Make the regression suite green again on the dev machine without hiding a
real rendering change: find why 13 of 16 spots differ from the references
that task 034's merge retook, then retake or fix.

## Context

- Found on 2026-10-22 while taking task 050's glow snapshots.
- With task 050's renderer changes stashed (the reverted build), a full
  regression run failed the same way: entrance 501, corrals 361, residents
  380, central-doors 133, north-rooms 396, map-edge 1085, klagraz-e0 74,
  klamall-e0 3235, klaratcv-e1 821, klaratcv-e2 3847, klatoxcv-e0 836,
  klatoxcv-e1 22777, klatrap-e0 2130 changed pixels; klacanyn-e0,
  klaratcv-e0 and klatoxcv-e2 pass with 0.
- The differences sit on sharp edges (a subpixel shift) and on klatoxcv's
  drums, which render visibly wider; floors, walls and the slime are
  pixel-identical, and two runs the same day are byte-identical to each
  other, so the state is deterministic today.
- Task 066 (2026-10-25) saw 13 mismatches and 3 passes; its base-engine
  control at `04dbdf88` reproduces all 13, and the 066 captures match that
  control exactly. Per-spot counts:
  `runs/066-dialog-black-background/baseline-comparison.json`. The entrance
  metadata also changed its triangle/page counts, so the audit must cover
  renderer and scene-cache changes, not only the shaders. (Folded in from
  the duplicate task 084.)
- The scene caches are byte-identical to the game's mod-cache copies, the
  game data files have no new timestamps, and the exe is main's code, so
  the cause is either the environment (the GPU driver changed?) or a merge
  that slipped past the suite. The references were last retaken by 034
  (8543064e).
- Klatoxcv elevation 1 and 2 snapshot runs also hung twice that day with no
  output while other maps captured; later the same spots captured fine with
  `-Still`. Suspected: a hidden modal box (debug.log has a "Program already
  running" line) when the owner's game was open. dev-snapshot could detect
  a modal box in a background run and fail fast instead of waiting out the
  timeout.
- Inbox 2026-10-26 (seen in 081): `tools/regression.ps1` fails 13 of 16
  spots on main's own cache (hydrants in corrals, klatoxcv-e1 22777 px,
  klamall-e0 3235 px); find which merge changed the game's view and retake
  the references with a before | after.

## Acceptance

- The cause is named in this task's `Log` (environment or code).
- `tools/regression.ps1` passes 16/16 again, references retaken only if the
  change is confirmed intentional.

## Log

### 2026-10-26 claude
Done: the cause is code, not the environment. The references 8543064e
(task 017, PR #34) retook were shot from a stale scene cache: its control
build at 32f0fe2b read a cache an older exporter had made, most likely the
owner's `game/mod-cache` (no `build/scene-cache` in that tree; entrance:
10101 wall triangles in 1 page, where 8543064e's own export makes 16094 in
3; klatoxcv's drums wider, `runs/058-regression-references-stale/`
left the stale reference, right main). 8543064e built with its own export
reproduces the references before it (8543064e~1) with 0 changed pixels on
all 16 spots, so 058's, 066's and 081's "13 fail, 3 pass" (the same counts
017 saw against the older set) is main against that stale set.
Main against the references before 8543064e, then a bisect of the merges
after it (each built with its own export): c8f93a43, 85c983b3 and 41373755
match 0 px; 40ffc835 (075, PR #107) changes scattered single pixels
(entrance 37, central-doors 31, north-rooms 43, residents 24, corrals 21,
klatoxcv-e0 281, klacanyn-e0 10, klatrap-e0 9; none on a thing), and main
equals 40ffc835 there. At those pixels the old and the new picture agree
with the game's own 2D picture (`-Classic`) equally often (entrance 12 old,
7 new, 18 neither), so it is draw-order noise from 075's new solid rows,
not a regression; 075's log saw the same. Every spot retaken from main
2332f28b with its own export, justified as: 8543064e's stale cache for all
16, plus 075 (40ffc835) for the eight spots above; the other eight differ
from the pre-017 set by at most 1 pixel.
`tools/regression.ps1` is a guard again: it exports the spots' maps with
the checkout's exporter into `build/scene-cache` before capturing (so an
exporter change shows, and no run reads a cache another build left), and
gives a capture that timed out one more try (corrals hung once on main).
Next: nothing.
Check: `powershell -NoProfile -ExecutionPolicy Bypass -File
tools/regression.ps1` passes 16/16 with 0 changed pixels on main's build.
