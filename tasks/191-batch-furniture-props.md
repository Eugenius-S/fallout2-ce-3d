---
id: 191
title: Furniture and props across every map, one rule per PR
milestone: M1
status: doing
owner: gpt
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Every piece of furniture and every prop (tables, beds, shelves, crates, barrels, junk) stands right in 3D: its shape, its footing on the floor, its cards and planes.

## Context

- The owner, 2026-11-04: geometry is fixed by kind, one class of objects
  across every map per task and per PR, never map by map.
- Sources: the finds of this kind in the area tasks' Logs (136, 158-171),
  the owner's screenshots (183) and the city reviews (175 on). Move each
  find here as a numbered line, then fix them by one rule.
- Find 1 (task 167, `runs/167-militarybase-in-3d/finds/t17131-classic-vs-3d.jpg`):
  at mbase34 e1, the `ca062.frm` beam on tile 17131 receives the glowing pool's
  green tint in 3D although the 2D object stays dark. Its wall geometry crosses
  hex rows 84 and 85; the game lights the object at its owner tile 17131.
  General rule: object-backed wall pieces use the owning object's map-tile light
  across the whole piece, while ownerless wall surfaces keep per-face lighting.
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
Done: added owner-tile sampling for object-backed wall pieces; the current
Release build succeeds and CTest passes 20/20. Matched 1920x1080 preview pairs
for mbase12 e0, mbase34 e1, and mbclose e0 are in
`runs/191-batch-furniture-props/`; JPEG sheets are below 1 MB and sidecars
record map, tile, builds, and limits. Pixel regression is 0/2,073,600 changes
on mbase12 and mbclose, and 89,438/2,073,600 on mbase34.
Not done: these are fixed wall-plus-hero previews, not a live fly-camera view.
The mbase34 beam band remains visually dark in both builds (mean green 33.23
before, 33.47 after); the bright floor patch in the reported crop also remains
unchanged. A `-Fly` capture still reported `static-wall-plus-hero-preview`, so
the reported green beam has not been reproduced or accepted as fixed. PR #308
stays draft; no runtime or regression claim beyond the three listed previews.
Next: obtain a comparable live view matching the owner's task-167 camera and
confirm which surface takes the green tint before refining the lighting rule.

### 2026-11-06 gpt
Done: merged current `origin/main` into this branch (`9ab4e977`) and rebuilt
Release successfully. Two fresh captures were attempted in isolated output
folders: `live-current-mainbase` used `-LiveFrame -NoPreview -Fly` and
`live-fly-diagnostic` used `-LiveFrame -Fly`. Both sidecars still report
`static-wall-plus-hero-preview` and a fixed camera; neither proves the
reported beam in the live fly view. `-LiveFrame` currently reaches the same
one-shot path as `-SnapshotOnce`, while `takeSnapshot()` rerenders an active
scene through `captureStaticPreviewPng()`. The `--dev-fly` activation issue
remains unexplained, and no lighting change is accepted from these frames.
Next: keep PR #308 draft until a real fly capture of the owner-reported beam
is obtained; investigate why the fly camera is not activated before retrying.
Check: Release build passed; capture sidecars and console logs are in the two
folders above. Existing 20/20 CTest and 3-map preview comparison remain from
the earlier run; these captures add no visual acceptance evidence.

### 2026-11-06 gpt
Done: merged the latest `origin/main` (`ad243e1c`) and ran the bounded fly tour
on mbase34 e1, tile 17131, at -45, 0 and +45 degrees. The tour produced real
camera-angle frames, but the reported green beam tint was absent in both main
and this branch; the before/after pairs are unchanged in
`runs/191-batch-furniture-props/fresh-live-owner-tile/mbase34-e1.jpg` (110 KB).
The green pool is visible, while the beam stays dark. Additional hour-8
snapshots without forced ambient still report the static-preview view and do
not count as live evidence. No lighting change is accepted; PR #308 remains
draft.
Next: obtain the owner's reported beam appearance in a live camera view before
accepting the lighting rule or expanding it across maps.
Check: Release build, CTest 20/20, exporter tests 135/135; the latest bounded
tour used identical scene-cache fingerprints and showed no before/after change.
