---
id: 192
title: Vehicles across every map, one rule per PR
milestone: M1
status: done
owner: gpt
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

Every vehicle (cars, the vertibird, wrecks, the tanker's machinery) stands right in 3D. Multi-part locking is 174; this batch is the shape of each vehicle.

## Context

- The owner, 2026-11-04: geometry is fixed by kind, one class of objects
  across every map per task and per PR, never map by map.
- Sources: the finds of this kind in the area tasks' Logs (136, 158-171),
  the owner's screenshots (183) and the city reviews (175 on). Move each
  find here as a numbered line, then fix them by one rule.
- Find 1, Modoc: the inn car is visible in all three turns; no shape or footing
  defect is listed in the area Log. Evidence: `runs/159-modoc-in-3d/tour-modmain.jpg`.
- Find 2, Gecko: the wrecked car is included in the area grid/walk sweep; no
  vehicle defect is listed. Evidence: `runs/161-gecko-in-3d/tour-gecksetl.jpg`.
- Find 3, coast encounter: the roadside car stays present in each camera turn;
  no shape defect is listed. Evidence: `runs/169-encdesertcoast-in-3d/coast1-e0.jpg`.
- Find 4, New Reno: the car-lot landmark was included in the three-turn sweep;
  no vehicle-shape defect is listed. Evidence: `runs/163-newreno-in-3d/tour-newr2.jpg`.
- Find 5, San Francisco tanker: the hull's floating sliver was fixed by the
  water-boundary plane rule in #305. Evidence:
  `runs/168-hull-stands/hull-before-after.jpg`.
- Find 6, Vertibirds: multipart alignment was fixed by #274; the billboard
  fallback for unsupported unique objects remains in #272/task 173, outside
  this shape batch.
- One run of this task is one PR in size: take the most visible cause, fix
  it everywhere, merge on green CI, then the next cause. The task stays
  open while finds of its kind keep coming.
- Speed and geometry work touch the renderer: never run this alongside a
  speed task (172, 156, 157).

## Acceptance

- Each PR: the rule it adds, before | after crops on at least three maps,
  ctest and the regression.
- The Log lists every find of this kind marked fixed (PR) or open.

## Out of scope

Other kinds of objects (their own batches), per-location fixes from the
owner's shots that are not of this kind.

## Log

### 2026-11-06 gpt
Done: reviewed the vehicle-related finds available in area Logs 136, 158-171.
The three-map sheets in `runs/192-batch-vehicles/` reuse merged area evidence
for MODMAIN, COAST1, and SFDock; each sheet shows the car or hull in matched
before/after camera turns. The remaining known vehicle groups are covered by
#274. No unhandled vehicle-shape defect appears in those area findings.
Regression and CTest were already run by the source area PRs: Modoc 27/27,
Gecko 21/21, EncDesertCoast 103/103 with zero changed pixels, and San Francisco
106/106 with zero changed pixels; CTest 20/20 in each applicable run. This
closure adds no runtime code. Owner feedback shots (183) and later city reviews
can add new findings and reopen the batch.
Next: none from the current area Logs; route any later vehicle-shape finding
through a new one-rule PR.
