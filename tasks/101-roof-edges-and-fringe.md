---
id: 101
title: Roof edges are clean, the roof fringe stands up like a wall, no wall tops through roofs
milestone: M1
status: doing
owner: gpt
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Seen from the air, a roof meets its walls cleanly: no dark steps along its
end, no wall tops poking through it, and the ragged fringe at a roof's edge
rises as a continuation of the walls instead of lying flat on the roof.

## Context

The owner, 2026-10-27, live with the 095 build, shots in `runs/inbox/`:

- `2026-10-27-roof-end-flaw.jpg` and `2026-10-27-roof-end-three-flaws.jpg`:
  along the end of the big thatched roof the dark wall top steps in and out
  under the roof's edge (one flaw on the first, three on the second).
- `2026-10-27-roof-horns.jpg`: dark wall-top strips stand up through the big
  brick building's roof (a broken dashed line, a black triangle at its right
  edge) and through the roof left of the bathhouse. Fewer than before 081,
  still there.
- `2026-10-27-roof-fringe-lies-flat.jpg`: the house with the ragged dark
  fringe (by the cow pen): in the original the fringe hangs down the wall
  top; in 3D it lies flat on the roof. It should rise from the walls like
  the walls do, as their continuation.
- Earlier roof work: 081 (roofs and overlaps), 071 (walls whole).

## Acceptance

- before | after from the air of the three roofs above and their original
  view at the same scroll, in `runs/101-roof-edges-and-fringe/`; the other
  Klamath roofs looked over the same way.
- The 090 seam totals do not rise; the original view regression stays pixel
  for pixel; ctest passes.

## Out of scope

Shadows on roofs, corner pieces (096), the walls' inside (102).

## Log

### 2026-10-30 gpt

Blocked before a code change: the current build shows the roof and facade at
tile 22885, but it does not reproduce the separate wall-top horns from the
owner's `runs/inbox/2026-10-27-roof-horns.jpg`. That owner screenshot came from
the 095 build and has no sidecar or camera tile. The separate flat fringe
symptom is visible in `runs/inbox/2026-10-27-roof-fringe-lies-flat.jpg`; I did
not treat it as the same issue.

- Refreshed the baseline build from `origin/main` `8e78b4e9` plus this branch's
  task-claim commit `a3c4d979`; source code is otherwise identical. Rebuilt the
  stale devtools translation unit. The executable SHA256 is
  `dfcf209b2aa25143d13da9e1b1b68c233a37600bd266307bc6d0aaee359f2032` and its
  sidecar reports `ce_commit: a3c4d979`.
- Exported Klamath scenes only to `build/scene-cache-main101`. The fresh
  tile-22885 3D frame and tile-19265 original-view frame, with matching
  identity sidecars, are in `runs/101-roof-edges-and-fringe/`.
- Tried `-FlyPitch`/`-FlyDistance`, `-TurnDegrees`, and `-NoPreview -Fly` in
  the background snapshot tool. Each still produced the fixed static preview
  camera, so none supplied the owner's flight angle. The owner game was not
  started, focused, or modified.
- No code was changed. Follow-up checks: `ctest --test-dir build
  --output-on-failure` passed 18/18. Re-exported all regression maps to the
  worktree cache `build/scene-cache` (not `game/mod-cache`) and captured the 16
  `tools/regression/check.py --list` spots twice with `-Classic -Still
  -SceneCache build/scene-cache`. Both passes report the same EXE SHA256 and
  build hash above; direct RGB comparison is 0 changed pixels across all 16
  1920x1080 frames. The source-code diff from `origin/main` is empty. The
  before/after roof acceptance, roof-wide review, and 090 seam comparison remain
  outstanding because no candidate fix was made.

Next: obtain a reproducible way to capture the actual fly-camera view (or its
camera parameters) from the owner screenshot, then isolate the wall-top
geometry at those same roof pixels before choosing a fix.

### 2026-10-30 gpt

Blocked after fresh-main fly and geometry checks. Merged `origin/main`
`3a59aefb` into this branch as `d09883b9`. The branch has no code changes from
that main commit.

- Rebuilt both checkouts. Main EXE SHA256:
  `e3332849deaba0d63b3fcc6ac57fab52eafc0ce07cb196e1366c46bccf63e987`;
  branch EXE SHA256:
  `aaf818079d7dae0b7761e9942cfa6120881b2b741c58dafed4a79cdf3b4e182e`.
  Their tour sidecars identify source commits `3a59aefb` and `d09883b9`,
  respectively. Both use 1920x1080, logical 960x540, scale 2.
- Exported `kladwtwn` independently to each checkout's `build/scene-cache`.
  Both provenance fingerprints are
  `86cb5a106cdebba03840f5647582dde92d8ef0717c58f348f22d5c7b23a03b4a`.
  No export wrote to `game/mod-cache`.
- Ran `tools/tour/tour.py` with `check_views` on five roof spots at turns
  -45, 0 and +45, then two broad roof spots at distance 28. All views passed
  map, tile and 3D-scene checks. Main and branch match at every spot because
  there is no code change. The fly views show the thatched roof by the cow pen,
  but do not match the exact wall-top fringe and horns in the owner's 095
  screenshots. The wide sheet is `fly-tour-owner-roofs-wide/kladwtwn-e0.jpg`.
- Read-only map and art inspection found the large roof near tile 22885 on roof
  cells (37-44, 55-60), with `grof037`-`grof041`, `grof079` and `grof087`.
  Wall rows nearby use dark top bands in `nec050`-`nec068`. This narrows the
  candidates but does not identify which pixels in the owner's image came from
  a particular donor.
- A diagnostic-only exporter copy under `build/diag-roofcap-exporter` removed
  horizontal triangles close to roof height inside roof coverage. Its separate
  cache reduced the Klamath wall sidecar from 29308 to 26199 triangles and
  changed the focused tile-22885 tour view. The thin roof-edge band remained,
  so this test does not establish the cause or justify a general fix. Its
  comparison sheet is `cap-triangle-diagnostic2/kladwtwn-e0.jpg`. No repository
  source was modified for this experiment.
- `ctest --test-dir build --output-on-failure` passed 18/18 on both main and
  branch. Full regression runs against their own explicit `build/scene-cache`
  had the same two reference failures on both checkouts: `klagraz-e0` 183
  changed pixels and `klaratcv-e2` 6299; the other 14 spots passed. Direct
  RGBA comparison of all 16 main and branch captures found 0 changed pixels.
  These failures are shared with current main, not introduced by this branch.

Next: get an owner screenshot with an exact map tile or camera pose for the
remaining horns and fringe, then trace those pixels before changing the
general wall or roof rule. Acceptance remains incomplete; keep this PR draft.
Check: the two `fly-tour` sheets and capture sidecars under `build/tour-101-*`,
the `cap-triangle-diagnostic2` sheet, and both full regression summaries.

### 2026-10-30 gpt

Blocked after a historical-build check: I could not reproduce the owner's
wall-top strips or flat fringe from a known camera pose, so I made no code
change.

- Matched the red brick roof and its surrounding buildings in
  `runs/inbox/2026-10-27-roof-horns.jpg` to Klamath Downtown. A p35/d12/yaw+15
  capture gives 340 SIFT matches and 139 RANSAC inliers against that owner
  image. This is a useful scene match, not an exact pose: the owner image has
  no camera or tile sidecar. The current `game/fallout2.cfg` says zoom 1.28,
  but it does not establish the setting at screenshot time; `-Zoom 1.28` also
  leaves this static preview pixel-identical to `-Zoom 1.00`.
- Built historical commit `dbc789af` (the 095 camera-boundary commit from
  2026-10-27) separately and exported its Klamath cache to
  `build/historical-095-src/build/scene-cache`. Its EXE SHA256 is
  `01ed469f6e765aa3d8a3d9251923c5a374e8a4f8b0525e0401fd93b7dee91793`;
  its sidecar reports `ce_commit: dbc789af`. Compared with this branch's
  log-only build from current main `3a59aefb` (EXE SHA256
  `aaf818079d7dae0b7761e9942cfa6120881b2b741c58dafed4a79cdf3b4e182e`), the
  same p35/d12/yaw+15 capture changes 22,898 pixels overall, 2,945 on the
  roof, and 10 inside the roof. Neither image contains the owner's dark
  segmented strips. The side-by-side is
  `runs/101-roof-edges-and-fringe/historical-vs-current-yaw15.jpg`.
- At p35/d12, separate no-row-616, no-row-617, and no-rows-616+617 caches
  produce identical images to one another; they do not explain the owner's
  strips. Removing all walls changes 387,183 pixels, removing roofs changes
  510,302, and the old roof-cap diagnostic changes 8,210 (7,197 in the roof
  ROI), but those broad controls do not isolate the cause. The older flat
  fringe image has only 4-6 inliers against the tested roof captures, so its
  correct camera/roof is also unconfirmed.
- No game/config/cache under `game/` was written. The prior current-main check
  remains the only regression result: ctest passed 18/18 on main and branch;
  each full regression had the same main-reference failures (`klagraz-e0`
  183 px and `klaratcv-e2` 6,299 px), and direct main/branch comparison was
  0 changed pixels across 16 views. No source code changed since those checks.

Next: obtain the owner view's exact camera/tile or a new capture with a
sidecar. Keep PR #168 draft and status `doing`; do not change the roof rule
until the old strips and fringe can be tied to specific projected geometry.
