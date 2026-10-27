---
id: 095
title: The 3D camera stops at the map's edge as the original does
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Scrolling the 3D view stops where the original game stops: at the map's
edge and at its scroll blockers. The player no longer slides the camera out
over the black past the map.

## Context

- The owner, 2026-10-27: today the camera is not held at the map's edge in
  3D; it should be.
- Why it is free now: the full 3D view turns on `tileSetFreeScroll`
  (`ce/src/scene/scene_view.cc`, "the 3D view scrolls past map edges"), and
  `tileSetCenter` (`ce/src/tile.cc`) then skips both the map-edge bounds
  (`mapEdgeTileInBounds`, the EDG files) and the object scroll blockers.
  Only the "keep near the hero" limit stays.
- First find out why free scroll was turned on (git log / blame of that
  line and its task) and keep whatever it was for; it may only be needed by
  the dev fly camera, which stays free.
- The 3D window is larger than 640 x 480 and the camera can turn up to 45
  degrees; the bound is on the view's centre tile, as in the original, so
  the edge on screen may show a little past the map. That is fine; the
  clean edges cut (016) handles what is drawn there.

## Acceptance

- In 3D, scrolling with the screen edges, the arrow keys and following the
  hero stops at the same centre tiles as in the original view (Ctrl+F10)
  on kladwtwn and one map with scroll blockers: shots of each edge in both
  views in `runs/095-camera-stops-at-map-edge/`.
- The dev fly camera still flies freely.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

Changing where the original game stops; the clean edges cut.

## Log
### 2026-10-27 gpt
Progress: Full 3D scrolling now honors map bounds and uses the original 480×400 hero-distance limit. KLAMALL exposed that the expanded 3D limit moved the center 240 px farther than Classic; removing it restores identical center stops. On final executable SHA `3f5e0256…`, all four KLADWTWN screen edges and arrows match Classic; KLAMALL EDG-enabled left edge also matches. Final captures and RGBA comparison are under `runs/095-camera-stops-at-map-edge/` and `build/095-final-regression-3f5e0256/`.
Remaining: Object-blocker branch, camera follow during actual walking, and interactive fly WASD remain unverified. The attempted real mouse walk moved the hero but left center unchanged; the test window is offscreen for WASD. See `runs/095-camera-stops-at-map-edge/verification.md`; leave task `doing`, `reviewed: no`.
Check: CTest 18/18; eight edge arrows and eight screen-edge probes passed; all 16 fixed 3D PNGs RGBA-identical to `094-direct-base` (baseline executable SHA `aad846b1…`, final `3f5e0256…`).
