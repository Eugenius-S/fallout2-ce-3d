---
id: 095
title: The 3D camera stops at the map's edge as the original does
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
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
Review: The owner accepted the change and closed the three investigation points: object blockers are unreachable on the installed EDG maps, walking follow is outside the original behavior, and the fly path is untouched. See `runs/095-camera-stops-at-map-edge/verification.md`.
Check: CTest 18/18; eight edge arrows and eight screen-edge probes passed; all 16 fixed 3D PNGs RGBA-identical to `094-direct-base` (baseline executable SHA `aad846b1…`, final `3f5e0256…`).

Follow-up investigation: `_dude_move` schedules character animation without setting the camera center, and `scene_view::refreshForPresent` mirrors `gCenterTile`; the tested click-walk therefore does not demonstrate auto-follow. Synthetic `SDL_PushEvent` key structs cannot hold WASD because `fly()` reads SDL keyboard state, which is updated by SDL's internal keyboard-send path. The configured `f2_res.dat` has 155 valid EDG maps with one zone per elevation; the current selector always picks that zone, so the object-blocker condition (EDG inactive or no selected zone) could not be reached on an EDG-loaded map in this archive. See the verification report for source references and limits. Removed the one-off blocker-search harness after preserving its negative result and the archive evidence.

Post-cleanup build SHA: `1becf6414fa63eaad52e413fcfb8f25e9aa65cc722de4af50b935c4ad17ea041`; CTest 18/18 and PowerShell parse pass. The cleanup only removes the dev-only blocker finder; the saved 16-pair RGBA captures still identify the prior renderer build `3f5e0256…` and were not rerun under this post-cleanup hash.
