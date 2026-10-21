---
id: 049
title: The see-through circle around the hero follows the zoom
milestone: M1
status: done
owner: glm
suggested: glm
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

The circle cut out of walls and roofs around the hero (the game's egg) has
the hero's size on screen at any zoom, as it has at zoom 1.

## Context

- The owner's find, 2026-10-21: zoomed in, the circle stays the game's size
  in screen pixels and cuts a small hole in the fence in front of the hero
  (`runs/049-egg-zoom/owner-egg-not-scaled.png`).
- The egg goes to the scene as a mask with its top-left corner on screen in
  game logical pixels (`setStaticEgg`, `ce/src/scene_renderer/static_scene.h`;
  `updateRoofsFromGame`, `ce/src/scene/scene_view.cc`). The zoom scales the
  game view around the screen center (`game_view.cc`), so the mask needs the
  same scale and the same center.
- The smooth zoom (042) changes the shown zoom every frame: use the shown one.

## Acceptance

- At zoom 0.5, 1 and 2 the circle sits on the hero and has the hero's size;
  snapshots in `runs/049-egg-zoom/`.
- At zoom 1 nothing changes: the regression passes as it is.

## Log

### 2026-10-21 glm
Done: the egg's circle follows the shown zoom. `bindEgg` hands the shader the
mask's rect scaled around its sprite anchor by the shown zoom (`GameView`,
already the smooth one), so the circle keeps the hero's on-screen size; at
zoom 1 the rect is bit-identical and the regression passes 16/16 with 0
changed pixels, ctest 11/11. Evidence in `runs/049-egg-zoom/`: the Buckner
house trio at zoom 0.5/1/2, on/off crops, the measured cut boxes double from
zoom 1 to zoom 2. Tooling: `--dev-zoom`/`-Zoom`, the egg block in the
snapshot JSON, `F2CE_NO_EGG` for on/off pairs, and `--dev-hero-tile` now
zeroes the egg's stale scroll shift (a teleport displaced the circle by up to
160 px in the snapshots).
Check: `tools/regression.ps1` (16 pass), `ctest --test-dir build` (11/11),
`tools/dev-snapshot.ps1 -Map kladwtwn -ViewTile 24285 -HeroTile 24285 -Zoom 2 -Still`.
Notes: the engine excludes `OBJECT_TRANS_NONE` walls (corral fences, the
Buckner house) from the egg; their 2D translucency is the wall-translucency
preference, which the 3D scene does not render yet. An earlier formulation
(extra zoom uniform in the shader) silently suppressed most of the wall cuts
on this GPU; scaling the rect avoids it.
