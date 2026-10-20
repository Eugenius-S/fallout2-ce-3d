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

The 3D map ends at the same playable boundary as the current installation's
Classic 2D map, with straight edges against black. The boundary may cut through
building or fence pieces that lie past the transition-zone corner.

## Context

- The owner's marked references are preserved at
  `runs/016-clean-map-edges/boundary-reference/owner-input/`.
- Current Classic 2D edge data comes from the active RPU `maps/*.edg` files.
  Its visible EDG rectangle is separate from the contracted scroll border and
  from the exit-grid tiles.
- The focused map evidence and transformed coordinates are in
  `runs/016-clean-map-edges/README.md`.

## Acceptance

- For the first two reviewed Klamath town maps, the 3D cut matches the
  current installation's Classic 2D EDG visible rectangle, transformed into
  world coordinates. Do not replace it with the floor-derived fallback.
- Floor, walls, objects, and picking share the cut. Click-to-move requests
  whose ground target lies outside the cut are rejected, while existing
  exit-grid transitions remain usable.
- Keep the existing `[3d] clean_edges` setting and menu label.
- After boundary behavior is accepted, update regression references with
  before and after images.

## Out of scope

Changing EDG data, collision, pathfinding, saves, or exit-grid locations.

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
Next: Superseded by the 2026-10-20 acceptance clarification: compare against the active Classic 2D playable boundary and capture town edges where that boundary crosses scenery.
Check: Release build passed; CTest passed 10/10; `git diff --check` passed. BLACK002 is a 160x168 full-fill card (palette 207 / RGB 12,12,12); MOUNT02 remains classified as normal scenery.

### 2026-10-20 gpt
Done: Rebased the fix on `origin/main`; the parent accepted the corrected KLACANYN floor-cut image. Release build and CTest passed 10/10; the parent independently verified scene-export tests 38/38 and regression tests 12/12. Prepared isolated on/off capture configs with the same Release EXE and `ddraw.ini`; their only `fallout2.cfg` difference is `clean_edges=0`.
Next: Capture matched authored/floor/off stills for KLADWTWN e0 view/hero 8711 and KLACANYN e0 view/hero 15909 at 1920x1080, scale 2; verify JSON camera and EXE identity; add the six PNG/JSON pairs and draft the PR after the shared helper queue clears.
Check: One corrected KLACANYN B image was captured with view tile 15909 and accepted by the parent. No final six-capture launch has started because another `fallout2-ce-test.exe` run occupies the shared queue.

### 2026-10-20 gpt
Done: Captured and curated five matched still PNG/JSON pairs under `runs/016-clean-map-edges/`. KLADWTWN has authored, floor, and off; KLACANYN has authored and floor. These are historical A/B evidence only. The owner clarified that the required boundary follows the original playable edge, even where it cuts scenery beyond the transition corner.
Next: Superseded by the 2026-10-20 acceptance clarification and current boundary-reference evidence.
Check: `tools/dev-snapshot.ps1` five still captures; metadata assertions passed for map, elevation, view/hero, camera, output size, source commit, and EXE hash. CTest 10/10, scene-export unit tests 38/38, regression unit tests 12/12, and `git diff --check` passed.

### 2026-10-20 gpt
Done: Rebased the implementation on `origin/main` `10fff6d5`, preserving zoom 015, lighting 020 part 2, and clean map clipping. The three conflict resolutions combined camera zoom with the map-projection accessor, required both lighting and clip shader uniforms, and retained all `Scene3dSettings` fields. A clean Release rebuild passed; CTest passed 10/10, scene-export unit tests 38/38, regression unit tests 12/12, and `git diff --check` passed.
Next: Parent reviews the boundary-reference evidence and decides the next behavior-verification step. KLACANYN off remains pending as a historical comparison capture.
Check: Current branch base is `origin/main` `10fff6d5`; five still captures remain pinned to source commit `f29410be` and the same EXE SHA-256. No snapshots were regenerated after rebase.

### 2026-10-20 gpt
Done: Reframed acceptance around the actual playable edge, not preserving all scenery. Read the active RPU EDG v1 rectangles for KLADWTWN and KLAMALL, verified authored 3D world bounds against runtime metadata, captured four Classic 2D corner references per map, and captured matched 2D/3D views at KLADWTWN tile 24768 and KLAMALL tile 13237. The latter shows the boundary beside a building in Trapper Town. Copied owner annotations and original PNG/JSON evidence to `runs/016-clean-map-edges/boundary-reference/`.
Next: Parent reviews the evidence. Audit and verify the outside-edge click command path and a real exit-grid transition before changing implementation or claiming those behaviors.
Check: Eight Classic corner snapshots and four matched edge snapshots report the requested camera tiles, 1920x1080 at 2x, and the same Release EXE SHA-256. At code level, `pickStaticGroundTile` applies `mapClipContains`, and `sceneTileAt` maps a failed ground pick to `-1`; the full movement command path and live exit transition remain unverified.
