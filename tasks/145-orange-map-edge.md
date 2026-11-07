---
id: 145
title: The 3D map edge matches the game, cut where it cuts, with no orange band
milestone: M1
status: reopened
owner: gpt
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The map edge batch. The 3D view shows nothing at the map's edge that the
game's 2D picture does not: no orange, dithered, see-through band over the
ground before the black, and the edge is cut where the original cuts it.

## Context

- Batch: the map edge (2026-11-04 inbox sweep). Folded in:
  - the inbox line of 2026-10-21: the 3D scene cuts the map edge farther out
    than the game (kladwtwn, the bottom edge by the cornfield,
    `runs/inbox/2026-10-21-map-edge-cut.webp`, the red line is where the
    original cuts). First screenshot the exit zones of the original maps to
    learn where the game cuts, then fit the clean edges (016) to that;
  - the inbox line of 2026-11-03: an orange translucent wedge at the map
    edge in fly shots, the same band as below.
- Found by Claude, 2026-11-01, on main `b056099a` in the background test
  run: `tools/dev-snapshot.ps1 -Map kladwtwn -Still -Hour 8 -ViewTile 24678`
  (and `-Zoom 0.5`). In the upper right, next to the black beyond the map, a
  wide band of ground is tinted orange with a fine dither pattern and ends in
  a slanted straight line: `runs/inbox/2026-11-01-orange-map-edge.jpg`.
- The snapshot JSON shows `map_clip` enabled, `source: authored`. Suspects:
  the map clip's edge fade, a scroll blocker or exit grid drawn as a tinted
  plane, a tile outside the authored bounds. Find which first.

## Acceptance

- kladwtwn's bottom edge by the cornfield is cut where the original cuts it
  (a shot next to the original's).

- The cause in the Log.
- before | after of the same view in `runs/145-orange-map-edge/`, plus the
  game's own 2D picture of it (`-Classic`); the fix is a general rule, not
  a special case for this map.
- ctest passes; the scene regression changes only at the map edge.

## Log

- 2026-11-01 Cut from Claude's 137 captures.

### 2026-11-05 gpt
Done: Traced the orange band to translucent exit-grid markers: a matched diagnostic capture with exit-grid cards hidden removes the band while preserving the same map, view tile, zoom, daylight and authored clip. Exit-grid objects now remain in the picking data but render as pick-only cards, so the transition stays available without drawing the marker overlay. Added dev snapshot logging for the active EDG rects and an `F2CE_DEV_CLEAN_EDGES=off` comparison mode. Release build and CTest passed (20/20); the fixed matched capture is in `build/task145-after/`, compared with `build/task145-3d-24678/` and `build/task145-classic-24678/`.
Next: Keep PR #293 draft while CI runs on the updated branch; the completed direct captures and regression evidence are in `runs/145-orange-map-edge/`.
Check: Release build and CTest passed 20/20. The full regression checker passed all 106 captured spots after updating only the 46 pixel-changing references. `tools/check_runs_size.py origin/main` and `tools/check_public_content.py origin/main` passed. The completed klatoxcv control fly tour and two failed Kladwtwn tour attempts are described below.
### 2026-11-05 gpt: regression/evidence follow-up

Captured the full 106-spot regression set at `runs/regression/20261105-113616-167-43892`. Comparing decoded RGB pixels against the old references found 46 changed spots; visual review of the complete changed-highlight sheet shows the differences at translucent EDG marker regions. The 60 unchanged spots have their original PNG/JSON references restored, avoiding unrelated baseline churn. The changed references now represent the general 3D rule that EDG cards remain pickable but are not rendered as translucent overlays.

Added matched before/after and Classic contextual JPEG evidence to `runs/145-orange-map-edge/`, with camera, resolution, time, clip, and executable hash metadata. Classic uses the vanilla framebuffer at game time 288005, so it is contextual rather than pixel-identical to the 3D pair at 288000. CTest passed 20/20 before the regression sweep. Next: run the regression checker against all 106 captures after the targeted reference restore, then verify CI on the merged-main branch and finalize PR #293 acceptance.
### 2026-11-05 gpt: fly-tour status

The full Kladwtwn fly tour did not complete on the baseline side: it produced 32 of 58 expected views, reached `tour_step=31`, then the harness timed out after 30 seconds without progress. A targeted Kladwtwn route with the two edge-area spots completed all 6 baseline views, but the branch side produced only 2 of 7 expected images and then timed out at 144 seconds. No further Kladwtwn fly-tour retry is planned after these two consecutive failures; the direct matched static before/after evidence and 106-spot regression remain valid.

A complete control tour of klatoxcv did finish (5 spots across three elevations). It reported 0 of 5 changed spots; the cave roofs and walls in those views contain no visible EDG overlay, so this is a no-unexpected-change control and does not prove the Kladwtwn edge effect. Its generated `changed.jpg` is retained with the evidence. PR #293 remains draft pending owner review of the missing completed fly-tour proof.

### 2026-11-05 gpt: stop after two failed Kladwtwn tours

Blocked: the full and reduced Kladwtwn fly tours both failed to complete,
including the branch-side reduced run. No further tour attempt is planned.
The code, matched direct captures, and regression sweep are complete, but the
map-specific fly-tour acceptance remains unverified. Keep PR #293 draft.
Next: return to this task only if a bounded Kladwtwn tour route or a harness
fix becomes available.
Check: the failure summaries and run paths above; do not treat the completed
klatoxcv control tour as proof of the Kladwtwn edge.

### 2026-11-06 gpt: refreshed-main edge verification

Done: Rebased the existing branch on current `origin/main` (`6beaf53f`) and
confirmed the exit-grid marker remains pickable while its translucent card is
hidden. A fresh targeted fly tour of Kladwtwn tile 24678 completed at -45, 0,
and +45 degrees on both main and this branch; the difference sheet shows the
orange marker removed at the two affected views and no change at +45. The
three-angle before/after sheet is `runs/145-orange-map-edge/tour-current-main-edge.jpg`.
The same-tile classic 2D view and current 3D still capture are in
`runs/145-orange-map-edge/current-time-8/`; their sidecars record 288005 and
288000 game ticks respectively, so they are adjacent contextual views rather
than a pixel-matched comparison. The existing matched static before/after
captures show the same authored clip bounds and the marker removal. Release
build and CTest passed 20/20; scene-export tests passed 135/135. The prior
full and reduced fly-tour timeouts are superseded by this successful bounded
route, but the regression references were retaken in this PR, so
`reviewed: no` remains until the required independent review.

Manual review: inspected both `runs/145-orange-map-edge/regression-review-*.jpg`
sheets covering all 46 changed before/after references. The changes sit on
translucent exit-grid wedges at map boundaries; regular geometry remains.
The other 60 references are pixel-identical. Confirmed `pickOnly` objects are
still submitted by `picking.cc`, and the exit-grid stays in the pick list for
the normal transition path.

Next: Verify the rebased head's CI. The PR remains draft until its CI is green.
Check: the current-main tour sheet, matched capture sidecars, build logs, and
test results described above.

## Log

- 2026-11-07: reopened by the owner's find: "when the camera moves into the
  3D mode the map loses its clean edges, they go cell-by-cell like the first
  iterations." Suspects from a first code read: the clean-edge clip resets
  while a map transition or camera change walks through
  updateMapClip (scene_view.cc), or a warm-cache scene built before the
  edge cut exists. The owner's scene.log has no fresh "clean map edges
  disabled" lines, so the clip-failure path did not fire; needs a live repro
  with logging on the transition itself.
