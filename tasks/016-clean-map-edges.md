---
id: 016
title: Clean map edges on black
milestone: M1
status: done
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

- On Klamath maps with an active EDG rectangle, the 3D cut matches the current
  installation's Classic 2D visible rectangle, transformed into world
  coordinates. Runtime references cover all seven maps at e0 and both cave
  maps at e1/e2. Do not replace authored EDG bounds with the floor-derived
  fallback where EDG data exists.
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

### 2026-10-20 gpt
Done: Parent review confirmed that move-mode mouse handling leaves the previous cursor tile in place when `tileUnderMouse` returns `-1`. PR #26 and the evidence README now describe the corrected playable-boundary target without presenting the floor-derived cut as preferred.
Next: Add a focused input guard, verify an outside-edge click cannot issue a stale movement command, then verify a normal exit-grid transition. Check the remaining Klamath maps with active EDG rectangles before marking the task complete.
Check: Code path inspected at `ce/src/scene/scene_overlay.cc`, `ce/src/scene_renderer/picking.cc`, and `ce/src/game_mouse.cc`; this is a code audit, not a runtime input test.

### 2026-10-20 gpt
Done: Move-mode mouse-up now rejects a fresh outside pick while the 3D scene overlay is active, and refreshes the hex cursor when the fresh valid pick differs from its cached tile. Movement is canceled if that refresh does not land on the fresh target. Classic movement remains unchanged.
Next: Parent reviews the guard. A live outside-click and exit-grid transition test remain pending; continue checking active EDG bounds on the remaining Klamath maps before marking task 016 complete.
Check: Release build passed; CTest passed 10/10; `git diff --check` passed. This is a code-path fix only; SDL input delivery, a live movement command, and an exit-grid transition remain unverified.

### 2026-10-20 gpt
Done: Audited the active RPU EDG records for all seven Klamath maps. Each has an e0 and e1 record; none has an e2 EDG record. KLARATCV and KLATOXCV have e2 map fixtures, so missing e2 clipping data does not mean the map elevation is absent. Added exact source rectangles and this caveat to the evidence README.
Next: Runtime visual references for the other five maps, a live outside-click movement check, and a reachable exit-grid transition remain pending.
Check: Read-only archive audit of active `mods/rpu.dat` (SHA-256 `27db830e0fb67d02b60e166e2acb772a92292f5f4dd88f733b740b406a0864ee`); no game or configuration files changed. No runtime probe was added, so movement and exit behavior remain unverified at runtime.

### 2026-10-20 gpt
Done: Extended the existing dev pick probe with an inside-movement control and a stale-cursor outside-click case. The probe chooses a reachable adjacent target, accepts a 2D-edge candidate only after the scene ground pick returns `-1`, then checks click-time and delayed hero/animation state.
Next: Verify one reachable local exit-grid transition and capture visual edge references for the other five Klamath maps. Task 016 remains open.
Check: Rebased on `origin/main` `91500779`; Release build passed; CTest passed 10/10. The isolated `tools/dev-snapshot.ps1 -Probe` run on KLADWTWN e0 passed: inside move 25075->25875, reachable stale target 25674, outside pick -1 at (956,51), synthetic mouse-up hero unchanged and busy 0->0, then delayed result `outside movement rejected`. Metadata and raw log are in `runs/016-clean-map-edges/movement-probe/kladwtwn/`; executable SHA-256 is `782ca2da4d04499592f26390097914d86ba2108fbdc9930bd6b967275589ce1e`. Primary `fallout2.cfg` hash remained `8D95CE8F0A11FDB975C2EAFE4836248DD4F9983074161BA4578EE463F2CCEF7F`; no owner process, mod cache, save, or configuration was touched.

### 2026-10-20 gpt
Done: Extended the existing `--dev-pick-probe` with a KLAMALL exit-grid stage. It validates the live exit object and destination, requires a fresh ground pick of the exit tile and a successful engine path, then queues a normal move-mode mouse-up. The probe does not call `mapSetTransition` or teleport the hero.
Next: Parent reviews the probe diff and runtime log. Visual boundary review for the remaining five Klamath maps is still pending; task 016 remains open.
Check: Release build passed; CTest passed 10/10; `git diff --check` passed. `tools/dev-snapshot.ps1 -Map klamall -Elevation 0 -Probe` passed the outside-click guard and the real transition: hero moved from 29889 toward exit 28078, live destination was map 9/tile 25142/elevation 0, and the engine arrived at `KLADWTWN.MAP` e0 tile 25142 after 59 frames. Raw log and the two same-run PNG/JSON snapshots are under `runs/016-clean-map-edges/exit-transition/klamall-to-kladwtwn/`. The captured dispatch line has a stale target value, documented in the README; the queued-exit and engine-arrival lines are authoritative. The logger now records dispatch coordinates. EXE SHA-256: `b5742fbd121a3bfbc2896ee256a5a65fe15e9d950059fe48cffef91638d12566`; `fallout2.cfg` remained at the expected hash `8D95CE8F0A11FDB975C2EAFE4836248DD4F9983074161BA4578EE463F2CCEF7F`.

### 2026-10-20 gpt
Done: Added matched Classic/authored e0 references for the five remaining Klamath maps and cave e1 references; existing cave e2 regression metadata also reports authored clip planes. Curated the good KLATRAP boundary view at tile 20733 and corrected the EDG archive audit. The earlier claim that e2 EDG rectangles were absent was incorrect.
Next: Attribute small non-edge regression differences before updating references. A targeted shader varying experiment will compare off-mode controls for central doors and one cave e1 view.
Check: Runtime evidence and exact source bounds are indexed in `runs/016-clean-map-edges/README.md`. Full on-run at `build/task016-regression/current-20261020-142049/summary.json` had 2/16 passes; off-mode controls at `build/task016-regression/off-20261020-142435/summary.json`, `off-interior-20261020-142627/summary.json`, and `off-e2-20261020-142823/summary.json` show some differences also occur with clipping disabled. No references were updated.

### 2026-10-20 gpt
Done: Corrected BLACK002 suppression to require the effective map clip to be active, even when the clean-edges setting is enabled. This preserves the boundary mask if authored or fallback clip setup fails. Updated all 16 regression references from the final authored-clip Release run and recorded the five pristine-main controls, before/after images, differences, and attribution in `runs/016-clean-map-edges/regression-attribution/`. All Klamath e0 maps and cave e1/e2 levels have authored EDG bounds; the visual edge set covers all seven e0 maps and both caves at e1.
Next: Parent handles the PR review response and merge. Task 017 remains separate.
Check: Final regression checker passed 16/16 with zero changed pixels at channel tolerance 2. Five pristine `origin/main` controls matched their clean-edges-off captures pixel-for-pixel; CTest passed 10/10, scene-export tests 39/39, regression tests 12/12, and `git diff --check` passed. Live outside-click rejection and normal exit-grid transition are verified in the evidence README.
