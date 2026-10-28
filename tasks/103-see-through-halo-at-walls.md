---
id: 103
title: The see-through halo round the hero works at walls again
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A wall in front of the hero turns see-through round him in the 3D view, as
018 made it, not only a roof.

## Context

- The owner, 2026-10-27, live with the 095 build: inside the bathhouse by
  its south wall the wall in front of the hero is drawn solid, no cut-out
  (`runs/inbox/2026-10-27-hero-see-through-halo-gone.jpg`); on a roof the
  halo still shows (`runs/inbox/2026-10-27-halo-on-roof.jpg`). So it is lost
  for walls only.
- 018 built it: `objectSeeThroughAroundDude` (`object.cc`) picks the
  objects, walls.bin names each face's map object. Something since then
  stopped walls from taking it.

## Acceptance

- Name the commit that lost it (git bisect with a dev snapshot of that
  bathhouse spot) and fix the cause.
- before | after of the bathhouse spot and of one more wall, in
  `runs/103-see-through-halo-at-walls/`; the roof case unchanged.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

The halo's shape and size.

## Log

### 2026-10-28 runtime diagnosis

- The 018 runner was isolated with a per-exe mutex and `F2CE_DEV_OUT`; the
  earlier 90 s no-snapshot run was fixed by passing an isolated config with
  `skip_opening_movies=1`. No unrelated process was stopped.
- The owner crop aligns to the static KLADWTWN reference, but does not identify
  the player's map tile. SIFT alignment to the 018 capture at
  `HeroTile=17665, ViewTile=17467` gave 41 inliers (median reprojection 1.15
  px); the marked wall point maps to about (602,677) in the 1920x1080 frame.
  The crop matches background features, not proof that 17665 is the owner hero.
- On the final 095 source (`20103388`, exe SHA
  `64acc550f01363feb7831394802165c770164a072287090d138478df48eae189`),
  `HeroTile=16306, ViewTile=17467` places the egg offscreen. On/off captures
  are identical (0 changed pixels). With `ViewTile=15300`, the same tile's
  game-view projection places the egg around the visible hero; with
  `ViewTile=15895`, the egg is visible and on/off changes 29,906 pixels in
  bbox (366,502)-(614,694), but the owner crop has no valid feature alignment
  to that camera view. These are not a valid before/after reproduction of the
  supplied scene.
- Coordinate probe for tile 16306: `gameViewTilePixel=(3552,428)` and
  `spritePixelToGround=(2.95833,-9.06250)`, identical to `StaticHeroInfo`.
  At view 15300, both hero ground point and egg tile center project to
  (413.89,319.17); the egg art offset moves its anchor to (413.31,346.08).
  At view 17467, both project to (-604.69,397.96), outside the frame. The
  apparent mismatch came from comparing this game-view matrix to the ordinary
  perspective camera printed in the sidecar; the scene is drawn with
  `gameViewMatrix`. This disproves a coordinate-transform mismatch for these
  tiles, but does not establish the owner's hero tile.
- `objectSeeThroughAroundDude`/`markStaticWallSeeThrough` and the two egg wall
  passes remain present. `staticScreenFromGamePixel` first appears in
  `a700f7fb` (task 010); no later removal/restore was found. No code fix or
  runtime bisect conclusion is justified yet. Remaining dependency: a
  reproducible owner state (hero tile and current view/zoom), or a valid scene
  fixture that puts tile 16306 and the marked wall in the matched crop.
- Temporary coordinate instrumentation was used only for these isolated
  runs and removed after recording these results. Raw captures and logs stay
  in `build/103-*`; `game/shots` and `game/logs` were not accessed.
