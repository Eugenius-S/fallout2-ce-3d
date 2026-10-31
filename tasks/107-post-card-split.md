---
id: 107
title: A corner post's card splits at its own axis, each half on the face it stands before
milestone: M1
status: doing
owner: gpt
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: [096]
reviewed: no
---

## Goal

The painted corner posts (the owner's shadow leftover 5 at the bathhouse's
alley corner and 6 at the pen's south corner) read the sun side of the face
they stand before at every hour: the post's art splits at the post's own
axis - the corner line it stands on - and each half takes the plane of the
face it continues, like every corner piece does since 096.

## Context

- 096's rule (a corner piece is cut at the corner line, each half on its
  own face's plane) is implemented through the span cut: a row whose span
  continues past the corner line is split there and the past stretch is
  re-homed onto the corner-line plane. At the pen corner that cut bites
  only the 2-pixel sliver rows 1176/1224, whose spans straddle the line;
  the post the eye sees is drawn by the fence rows 949/950, whose spans
  stop at or before the line, so their pictures carry the whole post on
  one plane and no span is left to cut (`runs/096-corner-piece-sun-side/`,
  the Log of 096).
- The bathhouse's painted post (leftover 5,
  `runs/inbox/2026-10-27-shadow-leftover-5.png`) is the same class: the
  post's pixels ride the picture of the wall row that the game camera
  overlapped it with, on that row's plane alone.
- The owner's direction (2026-10-28, on the 096 review): split a post
  card at its own axis by the same rule, if it can be done without
  one-off cases; otherwise it stands as its own work. The span cut cannot
  reach it, so this task exists.
- The same review left one more case of the class open: the corner piece
  in the turned perspective camera (the bathhouse corner piece lost,
  `runs/096-corner-piece-sun-side/inbox-corner-turned-before-after.jpg`).
  The turned camera draws the free camera's batches, where the piece rows
  stay trimmed at the corner line. Giving the free camera the cut pieces
  is the same rule applied to the other camera, and it may share this
  task's investigation of the free-camera corner machinery.

## Direction

Find where the visible post art lives in the donor pictures of the rows
near the corner, split that picture region at the post's axis (the corner
line mapped into the row), and give the past part the crossing face's
plane the way 096 re-homes a past stretch. The rule must hold for every
painted corner post of its kind, not one spot: name it in the PR and show
it on more than one post. Verify at the +-hours on the pen, the bathhouse
alley post, and at least one more post of the same kind on another map.

## Acceptance

- The turned perspective camera keeps the bathhouse's south corner piece
  where the isometric camera shows it
  (`runs/096-corner-piece-sun-side/inbox-corner-turned-before-after.jpg`,
  tile 18930, zoom 2.5, turn 40, hour 8), and every other corner piece of
  its kind with it: the free camera gets 096's cut pieces.
- Posts 5 (bathhouse alley) and 6 (pen) and at least one more painted post
  on another map read their own face's side of the sun at hours 8 and 17:
  before|after sheets in `runs/107-post-card-split/`.
- The 090 seam totals do not rise beyond 096's accepted ones; the fly tour
  against main changes only at corners; the regression is 0 px on the
  unchanged views, the changed ones with new references and diffs.
- ctest and the exporter tests pass.

## Out of scope

The bottom-edge shadow step by the pen (tile 28096, outside 096's class,
in the inbox), new shadow features.
## Log

- 2026-10-28 glm: cut from the 096 review. The span cut cannot split the
  visible post art (the rows that carry it have no span past the corner
  line); the art-level split needs its own investigation of the donor
  pictures and the free-camera corner machinery.

### 2026-10-30 gpt

Done: Implemented an alpha-run-based post-card split and paired-wall plane assignment in the exporter. Candidate geometry was found on KLADWTWN (fence rows at tiles 11281 and 26074; wall rows at 17304 and 26112) and KLAMALL (wall row at tile 13250), across two prototypes. Exporter tests passed 69/69; ctest passed 18/18.

Check: Runtime h8/h17 pairs show corner/post changes at the pen, bathhouse, two other KLADWTWN candidates, and KLAMALL. The tour sheets show visible wall/fence corner-region changes; 39/57 KLADWTWN and 4/21 KLAMALL view/turn pairs differ, with other pairs at 0 px. Some changed views cover broad wall regions, so this is not a pixel-by-pixel proof that every changed pixel is intended. Tour sheets: `runs/107-post-card-split/tour/changed.jpg` and `changed-2.jpg`.

Matched 090 seam totals: 17/18 unchanged; KLADWTWN tile 17104 at h17 drops from 237 to 230; no total rises. Regression references were not changed. Direct 3D preview comparison is 13/16 exact; the remaining three views differ by 2, 2, and 4 px (8 px total). The same three classic captures are 0 px.

Next: Free/turned-camera acceptance at bathhouse tile 18930, zoom 2.5, turn 40, h8 remains unverified. Two attempted capture routes reported fixed-eye metadata and produced identical before/after frames. No claim of passing this acceptance; draft PR pending a valid turned-camera capture and clean disposition of the 3D preview differences.

### 2026-10-30 gpt follow-up

Done: Merged fresh `origin/main` (`3a59aefb`) into this branch (`e6cc7d68`), rebuilt both Release executables, and confirmed `ctest` 18/18 and exporter tests 69/69. On three direct, same-cache 3D previews the branch differs from main at 8 pixels total (2, 2, and 4); the matching classic captures are pixel-identical. The 8 pixels are localized to post-like wall details, but are not proven to be the intended pixels. No references changed.

Check: The custom fly tour captured the actual camera at tile 18930, zoom 2.5, turn 0 and turn 40. Sidecars show `hero.rotation` 3 and 4, `scene_view=full`, and `renderer=scene-gl`. With the submitted exporter code, main and branch at turn 40 are identical: the `piece` rows have `keep=(1,1)` and are omitted from the free-camera batch. Passing the same corner split to an omitted row did not change `walls.bin` or the rendered frames. A temporary, reverted diagnostic that restored only rows 618/619/1149 from `nec040.frm` (map object tile 19132) changed 3,504 pixels at turn 0 and 1,286 at turn 40; restoring every piece row produced an identical target turn-40 image. The donor crop shows a striped post with a boss, matching the description in the 096 owner reference, but the changed render still reads as a narrow strip at the wall seam and does not visually confirm the full post and boss. Diagnostic sheet: `runs/107-post-card-split/verified-custom-fly-pillar-only/kladwtwn-e0.jpg`.

Next: Find a general free-camera rule that restores this exact post without duplicating or obscuring the corner fills, then verify its full silhouette at turn 40 and on other corners. The target acceptance and the 3D preview pixel differences remain unresolved; keep this PR draft.

Check: `ctest --test-dir build -C Release --output-on-failure`; `py -3 -m unittest discover -s tools/scene_export -p 'test*.py'`; custom `tools/tour/tour.py` run with `--before build/compare-main-069-worktree`, the explicit diagnostic cache, and `-Tour kladwtwn:0:18930:-1:0:2.5,kladwtwn:0:18930:-1:40:2.5`.

### 2026-10-30 gpt post/occlusion diagnostic

Done: Compared the donor `nec040.frm` with the owner's 096 reference and the isolated runtime render. The donor's circular boss and base match the post in the left isometric owner panel. The isolated turn-40 render contains that same art, with a large exposed wall-plane artifact because neighboring walls were removed. Two cache controls used the same tour and executable: removing the roof layer from the full scene produced 0 changed pixels at the target views; retaining only map wall object 2247 (tile 19132) with the roof removed exposed the donor post/boss. This rules out the roof as the cause in these views, but does not identify a safe production fix.

Check: The target's nearby rows are 618, 619, 1149, and 1216 (owner 2247; thickness 0.2227-0.263). `_corner_fits` marks the first three as corner pieces with `keep=(1,1)`; row 1216 retains a partial interval. `_corner_post_homes` skips rows at or above `CORE_THICK=0.15`, so the thick target art is outside that alpha-run path. Adjacent wall rows 616/617 belong to owners 2245/2246 and remain possible occluders; they have not been isolated individually.

Evidence: `runs/107-post-card-split/isolated-post-no-roofs/kladwtwn-e0.jpg` and `runs/107-post-card-split/no-roof-layer-control/`. The disputed full silhouette at turn 40 and the multi-corner acceptance remain unverified. No production code or references changed; keep this PR draft.

### 2026-10-30 gpt neighbor-face isolation

Done: Captured full, no-row-616, no-row-617, no-both, target-object-only, and target-plus-one-neighbor caches with the same EXE, game time, view tile 18930, zoom 2.5, and turns 0/40. Removing row 616 (object tile 19130) changes 0 pixels in the post ROI; removing row 617 (tile 19131) changes 75,613 ROI pixels at turn 40. In the isolated caches, object 2247 alone has no post image at that corner, adding object 2245 does not restore it, and adding object 2246 does. The 2D game-camera captures with full and no-row-617 caches are pixel-identical.

Check: Rows 618/619 of object 2247 are already `corners.pieces` and `decor`; `_corner_decor` folds their art onto both faces. Row 617 is the adjoining plain wall face that supplies the second plane in the isolated test. Removing the thickness prefilter from `_corner_post_homes` in a temporary exporter copy produced 0 changed pixels at the target turns and still found no homes on rows 618/619/1149/1216; it produced candidates on unrelated rows 1118/1134, so that relaxation is not safe. The current-main exporter files were compared with the cache exporter (`8e78b4e9..3a59aefb`, no `tools/scene_export` diff); current-main and branch caches shot with the same EXE are 0 px different at both target turns. This exact owner symptom remains unreproduced; do not claim it fixed or change code based on the row-removal artifact.

Evidence: `runs/107-post-card-split/occlusion-owner-617-comparison.jpg`; raw isolated captures and JSON sidecars remain in the corresponding `occlusion-*` and `post-plus-*` folders. EXE SHA-256: `aaf818079d7dae0b7761e9942cfa6120881b2b741c58dafed4a79cdf3b4e182e`. No production code or references changed; keep this PR draft.

### 2026-10-30 gpt owner-view clarification

Correction: The earlier statement that the isolated object-only runtime frame showed the complete donor post was too strong. Same-camera pixel comparisons show object 2247 alone does not render the post; adding adjacent object 2246 (tile 19131 / row 617) restores the boss/base image, while object 2245 (tile 19130 / row 616) does not. The neighboring row supplies the second wall plane used by `_corner_decor`, rather than being an unrelated occluder.

The current-main exporter cache was generated from the main worktree's exporter; `tools/scene_export` is unchanged between `8e78b4e9` and current `origin/main` `3a59aefb`. With the same EXE SHA above, main and branch captures at tile 18930, zoom 2.5, turns 0/40 are 0 px different. Both captures visibly show the post texture, unlike the owner's annotated 096 turned-camera reference. The matching 2D captures are also exact (`9eca4a5d3f0ad5d4db3f356c7b8c757130b00c92e4accd64b030b6ca2170404c`). This comparison does not reproduce the owner symptom or prove the entire requested silhouette; keep the acceptance unresolved.

Check: The alpha-run diagnostic with only the thickness guard removed left target turns unchanged and found no post homes on the target's split sprites (17x111, 15x108, 2x110, 2x110). It instead found unrelated thick candidates at rows 1118/1134. No production code, references, or status changed. The existing 8 px 3D preview differences and multi-post/hours acceptance remain open; PR stays draft.

### 2026-10-31 gpt current branch and thick-decor diagnostics

Check: Re-exported fresh current-main and branch caches with explicit build cache roots. The same `c7af38c2...` executable (sidecar `ce_commit=b8eb43c5`) passed `tools/tour/tour.py`'s `check_views` at KLADWTWN tiles 11281, 26074, 17304, and 18930, and KLAMALL tile 13250; each actual full-scene fly pair at turns 0/40 differs. At tile 18930, zoom 2.5, turn 40, the current main and branch captures are pixel-identical and still show only the narrow edge at the corner, not the owner-reference pillar. Static-camera h8/h17 captures at tiles 17304, 26074, and 13250 show the views and changing sun direction, but do not establish that the requested post halves read the correct face. Direct static-camera main/branch comparisons at tile 17304 are 0 px both with `-Shadows` off and on; the post rehome is fly-only, so it currently does not change that camera's shading.

Diagnosis: The thin-post candidate rows 433 (KLADWTWN object tile 17304, sprite 1895, 27x115, thickness 0.06) and 192 (KLAMALL tile 13250, sprite 1211, 28x117, thickness 0.06) get `keep=(1,1)` from `_corner_fits`, so their posts are fully clipped in the fly wall pass. The pen post row 949 at tile 26074 has a non-empty keep interval and is the only one of these examples with a small visible turn-capture change. The thick bathhouse corner uses rows 618/619 (sprites 1946/1947, 17x111 and 15x108; thickness 0.263/0.2227); they are `corners.pieces` and `decor`, so the thin alpha-run detector does not apply. `_corner_decor` selects a front using the fixed isometric camera's `stands`/nearness test.

Temporary diagnostic caches (no production thick-decor change retained) tested three face-assignment variants against the same real turn 0/40 tour and controls at 17304, 26074, and 11281. The color-difference mask from plain wall columns marked row 618 columns 11-16 and row 619 columns 0-12, but the first mask-axis experiment added no geometry boundaries and changed 0 px. Forcing all visible decor sections onto the twin face changed 49,716 px at turn 0 and 161,919 px at turn 40, while controls were 0 px; it produced a detached wall panel. Adding boundaries at the mask bands' midpoints (13.5 and 6.5) changed 108,323 px at turn 0 and 46,456 px at turn 40, again with controls at 0 px; it produced detached decorative strips rather than the requested pillar. The fixed-camera face selection is implicated, but neither tested mapping is visually valid. Sheets: `runs/107-post-card-split/thick-mask-axis-diagnostic-20261031/comparison.jpg`, `runs/107-post-card-split/thick-twin-diagnostic-20261031/comparison.jpg`, and `runs/107-post-card-split/thick-axis-split-diagnostic-20261031/comparison.jpg`.

Check: Exporter tests pass 70/70 and ctest passes 18/18. The last complete regression against the existing reference set was 14/16; the unchanged main/branch source camera cases remain exact, while two stale references differ by 183 px (`klagraz-e0`) and 6,299 px (`klaratcv-e2`). References were not changed. The post 5/6, third-post h8/h17 visual acceptance and the turned bathhouse pillar remain unverified. Keep PR #170 draft and task status `doing`.

### 2026-10-31 gpt exact-mask thick-decor check

Check: In a temporary cache only, routed the full atlas-confirmed column bands of thick wall rows 618/619 (owner 2247) to the perpendicular wall face: row 618 columns 11-17 and row 619 columns 0-13. All other source columns kept the existing face selection. The same isolated fly tour used the current test executable (SHA-256 `c7af38c23acc9d198e1a3e2543a75faafbd04860734e3e16910592828eecdb4f`, sidecar `ce_commit=b8eb43c5`), target tile 18930 at zoom 2.5, turns 0/40, and controls 17304, 26074, and 11281. The target changed 37 pixels at turn 0 and 0 at turn 40; all three controls were unchanged. The turn-0 difference is a narrow seam stripe and does not restore the post/boss silhouette. The mask-band routing hypothesis is rejected; no production change was made. Comparison: `runs/107-post-card-split/thick-exact-mask-diagnostic-20261031/comparison.jpg` (234 KB). Keep task status `doing` and PR #170 draft.
