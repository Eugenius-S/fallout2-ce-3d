---
id: 016
title: Clean map edges on black
milestone: M1
status: doing
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: [014]
reviewed: no
---

## Goal

The map ends in straight edges on a black background instead of the ragged
teeth of diamond-shaped tiles.

## Context

- Floor tiles are diamonds, so the outer border of a map is jagged.
- Many maps limit the visible area: `ce/src/map_edge.cc` (Restoration
  Project's map edge rectangles). In 3D these limits are switched off for
  scrolling (`mapEdgeIsOverClippedArea`), but they still describe where the
  map is meant to end.

- The owner's target: `runs/016-clean-map-edges/owner-target.webp` (left:
  now, the teeth of the diamond tiles; right: wanted, the map's sides cut as
  straight lines through the outermost tiles, black beyond).

## Acceptance

- The scene is cut along straight lines: the map edge rectangle where the map
  has one, otherwise the straight envelope of the floor. Floor, walls and
  objects beyond the cut are not drawn; beyond is black.
- Before finishing, show the owner both variants of the cut on two Klamath
  maps (snapshots in the PR) and keep the chosen one.
- `[3d] clean_edges` (default on) with a menu label.
- Regression references updated with before and after images.

## Out of scope

Changing what the player may scroll to.

## Log

### 2026-10-19 gpt
Done: Claimed task 016 on main; dependencies are done.
Next: Inspect the target image and clipping/render architecture, then implement two reviewable map-coordinate cut variants.
Check: `tools/tasks.ps1`; task branch will follow this claim.

### 2026-10-19 gpt
Done: Added default-on `[3d] clean_edges`, strict default-preserving config reads, per-key scene revision access, authored EDG/floor clip geometry, shared draw and picking cuts, and A/B snapshot metadata.
Next: Parent reviews this implementation before two-map variant captures.
Check: Release build passed; `ctest --test-dir build --output-on-failure` passed 9/9; `git diff --check` passed.

### 2026-10-19 gpt
Done: Added the pure map-clip geometry suite as a standalone CMake/CTest target and CI matrix entry.
Next: Capture matched authored/floor variants and the clean-edges-off baseline on both Klamath maps for parent review.
Check: Standalone Release build passed; `ctest --test-dir build-tests-map-clip-ninja --output-on-failure` passed 1/1; `git diff --check` passed.

### 2026-10-19 gpt
Done: Replaced the rejected floor-grid bounding box with a straight floor cut derived from occupied cell centers in the cached game projection, inset by half a projected cell. Floor-only debug metadata now identifies its grid fallback.
Next: Retake matched authored/floor/off screenshots on both Klamath maps and ask parent to review the corrected floor cut.
Check: Release build passed; CTest passed 9/9 including the projected diamond-mask geometry cases; no regression references were changed.

### 2026-10-20 gpt
Done: Traced KLACANYN's remaining gray staircase to RPU's opaque BLACK002 boundary-mask cards. Clean edges now omits only scenery resolving to `BLACK002.frm` before creating render and pick entries; added focused path/type behavior coverage. The matching opaque BLACK003 card is absent from the effective scenery list and both reviewed maps.
Next: Retake KLACANYN's floor-cut snapshot at view tile 15909 for review, then capture matched A/B/off variants on both maps after the parent accepts the corrected B image.
Check: Release build passed; CTest passed 10/10; `git diff --check` passed. BLACK002 is a 160x168 full-fill card (palette 207 / RGB 12,12,12); MOUNT02 remains classified as normal scenery.
