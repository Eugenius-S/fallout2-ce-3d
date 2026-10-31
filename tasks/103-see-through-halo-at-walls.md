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
- The owner again, 2026-10-28, live on main `9705aca4`
  (`runs/inbox/2026-10-28-halo-roof-only.png`): the hero stands behind the
  brick building's wall (the red hex), seen from the camera through the
  wall; the roof shows the cut-out, the wall under it stays solid. The
  first try (2026-10-28) put the hero above the wall's top edge in frame,
  where no wall stands in front of him: the spot to test is a hex behind a
  wall's line from the camera, the wall between the camera and the hero.
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

### 2026-10-28 owner-point comparison

- Mapped the owner image's lower figure to the KLADWTWN game view and used
  `pickStaticGroundTile(243, 320)` at `ViewTile=17467`: `HeroTile=17878`,
  world=(-10.94009,-5.06141). The marked south-wall object is row tile 19102,
  fid `030001BB`. The target is the bathhouse room behind its south wall, not
  the standard preview locations.
- Captured full 3D at 1920x1080 in separate worktrees: 095 commit `dbc789af`
  (exe SHA `9bd5c8f9785958cfee883833fd5324b59c67e55df6be298d1eea2d97bcabb849`)
  and fresh `origin/main` commit `623e380c` (exe SHA
  `d8661d7f115f49c62b61de90273e38d29e3d11632d99cff425d3b5e8cb8ccfa3`).
  Both use `KLADWTWN e0`, HeroTile 17878, ViewTile 17467. Sidecars agree:
  world anchor=(-11.04167,-5.06250), egg rect=[183,249,129,98],
  628 wall rows marked. Decoded RGBA comparison is 0 changed pixels. In both
  frames the marked south-wall section remains solid; the owner's wall symptom
  is present on both revisions, with no 095-to-main behavior change.
- Mapped the separate roof reference to `HeroTile=21263`, `ViewTile=17467`.
  The feature match to the 038 q00 panel had 20 inliers; this is a lower
  confidence control. Captures on the same two revisions have identical
  metadata and 0 changed RGBA pixels; the hero is on the roof edge with the
  egg visible.
- Artifacts are in the isolated worktree `build/103-owner-target-095/`,
  `build/103-owner-target-main/`, `build/103-owner-roof-095/`, and
  `build/103-owner-roof-main/`. No game source was changed; task remains
  `doing`, `reviewed: no`. This targeted comparison does not identify when the
  behavior changed between 018 and 095, so it is not a completed fix or bisect.

### 2026-10-28 current-main exact-point check

- Built `origin/main` at `47148c1c` and captured one isolated still snapshot of
  KLADWTWN e0, `HeroTile=17878`, `ViewTile=17467`, at 1920x1080. The capture
  reports the egg active (`rect=[183,249,129,98]`, 628 wall rows marked). The
  frame is `build/103-owner-target-current-main/devhero-20261028-173652-000.png`
  in the isolated current-main worktree; exe SHA-256 is
  `9879c59585578bf67c6ecc73f8555a075a4f094892064a27b0167577bddf1612`.
- Compared the hero and the south-wall edge in the supplied crop and the fresh
  frame. The hero's feet remain above the wall's top edge, so the wall does not
  occlude the hero in this captured view. The marked wall section is also away
  from the hero silhouette. This does not establish a reproduced see-through
  regression, so no bisect or code change is justified from this run.
- Status: blocked pending a reproducible view where the south wall actually
  crosses the hero or the intended egg area. The owner's game and its files
  were not touched.

### 2026-10-28 gpt: current main wall overlap

Built the current main merge and captured KLADWTWN elevation 0 at hero and
view tile 19075, with and without `F2CE_NO_EGG=1`. The bathhouse front wall
occludes the hero with the egg off and reveals him with it on. The matched
crop is `runs/103-see-through-halo-at-walls/bathhouse-19075-egg-off-on.jpg`.
Nearby hero tiles 19071, 19073, 19077 and 19079 also show an egg on the wall.
This proves the mechanism works at these locations, not that the owner's
reported brick-wall and bathhouse locations work. The regression point, good
and bad bisect endpoints, and cause remain unidentified; no fix was made.
Next: find a wall-overlap point reproducing the reported failure, compare it
with 018 using its matching scene cache, then bisect that same point.
Check: `tools/dev-snapshot.ps1 -Map kladwtwn -ViewTile 19075 -HeroTile 19075
-Still`, once normally and once with `F2CE_NO_EGG=1`.

### 2026-10-28 gpt: owner settings and saved game probe

Read the owner's configuration without changing it: 2310x1298, scale 2,
3D zoom 1.024, camera rotation 0. `dev-launch.ps1` derives the window from
monitor work area and requests 16:9; it was not run. Copied Slot 1 into an
isolated test output directory and loaded that copy with the same resolution,
scale and zoom. At capture the hero and egg both had tile 19086 and offsets
(0, 0); the snapshot reported egg rect (378, 280, 129, 98) in the logical
1155x649 view. This save's hero was outside the bathhouse, not at the owner
screenshot's wall, so it does not reproduce the failure.

A separate walking probe at the same resolution and zoom moved the hero from
19086 to 15284; at the end both hero and egg had tile 15284 and offsets
(0, 0). The first walking probe from 19075 could not find a path. The test
harness resets the egg offsets when it teleports the hero; the saved-game
probe did not use that teleport. Temporary diagnostics were removed.

The 3D egg anchor uses `gameViewTilePixel(gEgg->tile)` plus FRM and object
offsets, projected by `staticScreenFromGamePixel` into the logical viewport;
`bindEgg` applies zoom to its dimensions. The hero card uses the hero tile and
movement offsets in the same game projection. These checks have not isolated
the reported displacement or verified a fix. Next: reproduce the wall case
with the live camera and actual movement, then measure hero and egg positions
in the same frame at both aspect ratios and zoom levels. No code fix retained.

### 2026-10-29 gpt: camera matrix and reproduction limit

- Merged current `origin/main` through `d12044de`. The task branch has no
  renderer, exporter, or regression-tool changes relative to that main.
- Captured the wall overlap at hero/view tile 19075 in 16:9 (1920x1080) and
  4:3 (1440x1080), scale 2, at owner zoom 1.024 and doubled zoom 2.048.
  Sidecars identify build `e46846e3`, EXE SHA-256
  `86248764df0d1493c241df448f97be91321378bb34bcec9b8cd65661532ca03b`.
  Egg on/off diffs are 33,004, 132,461, 33,005 and 132,461 pixels; each
  bounding box follows the projected egg across both aspect ratios and zooms.
  Four hero crops and exact capture metadata are in
  `runs/103-see-through-halo-at-walls/accepted/`.
- Tile 19102 at view 19075 is offscreen (zero changed pixels), so that is not
  a valid comparison. Centering view on tile 19102 gives 19,888 changed pixels
  at both aspect ratios and zoom 1.024, but still does not establish the
  exact red-marked location in the supplied 2026-10-28 image. Its save state
  and camera metadata are unavailable; feature matching did not map the mark
  to a reliable tile. No first-bad commit or code cause is established, and
  no renderer change or reference retake is justified.
- CTest passed 18/18; scene-export Python tests passed 67/67; regression
  Python tests passed 12/12. Full `tools/regression.ps1` passed 14/16: 183
  changed pixels at `klagraz-e0` and 6,299 at `klaratcv-e2`. These are
  current-main/reference mismatches, not caused by this task branch; refs
  were not retaken.
- Next: obtain a reproducible camera/player state for the marked wall, then
  compare that exact point against 018 and bisect before changing code.
- Check: build `e46846e3`; CTest 18/18; Python 67/67 and 12/12; full
  regression 14/16 with the two mismatches above.
