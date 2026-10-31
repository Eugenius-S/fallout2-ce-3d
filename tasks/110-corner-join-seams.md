---
id: 110
title: No thin seam where two walls meet at a corner
milestone: M1
status: doing
owner: gpt
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: [107]
reviewed: no
---

## Goal

Where two walls meet at a corner, the corner reads as one solid edge from
every camera: no thin light or dark line down the join.

## Context

The owner, 2026-10-28, live on main `9705aca4`, Klamath:

- `runs/inbox/2026-10-28-stop-corner-seam.png`: the brick building by the
  STOP sign: a thin light vertical line runs down the corner, from the
  roof edge to the ground. The same corner was 01 of the 2026-10-25
  review (`runs/inbox/2026-10-25-review2-01..12.png`); 096 changed it but
  the line is left.
- `runs/inbox/2026-10-28-dark-corner-seam.png`: a dark house's corner
  (the red mark): a small seam at the join, near the eaves.

The corner pieces are cut at the corner line since 096 and the post cards
split there in 107, so this task goes after 107 and checks what is left.
The 090 seam metric counts sun-shadow seams, not geometry gaps; this one
may be a gap between the two faces' meshes at the corner line, or the
background showing through an alpha edge there.

## Direction

First find, for both shots, what the line's pixels are: the background
through a gap, the other face's back side, or a texel from outside the
picture. One rule at the corner line for all corners, no per-map cases.

## Acceptance

- before | after of both corners and two more corners of other buildings
  in `runs/110-corner-join-seams/`, isometric and turned camera.
- The 090 seam totals do not rise; the fly tour against main changes only
  at corners; the original view regression stays pixel for pixel; ctest
  and the exporter tests pass.

## Out of scope

Shadows on the corner pieces (109), the post split itself (107).

## Log

### 2026-10-30 gpt independent diagnosis attempt
Claimed the task to check whether the STOP wall corner can be investigated apart
from task 107. Captured current-main 3D snapshots at `klamall:13050` and
`kladwtwn:17104`, at hero rotations 0 and 40, plus classic 2D and a
shadows-on control. Captures use the clean Release EXE and `build/scene-cache`;
identities and compact images are in `runs/110-corner-join-seams/`.

The STOP location is a wall-wall corner without an obvious separate painted
post in the owner's image, so its wall edge can be investigated independently.
The dark-house corner includes a decorative post/card and overlaps task 107.
`corner_rays.py` found no through-rays at either exact target view tile; this
rules out a large sampled mesh opening only, not a subpixel gap or texture edge.
The broad shadows-on/off image differences did not isolate the seam pixels.
The 0/40 images differ, but the capture sidecar still labels its camera static,
so the camera angle itself is not accepted as proven. I did not identify
whether the remaining line is alpha, texel color, or a narrow raster gap and
made no code change.

CTest on this unchanged source passed 18/18. Fresh-main self-regression using
`build/scene-cache` passed 14/16 references; `klagraz-e0` differed by 183
pixels and `klaratcv-e2` by 6299. This is not an independent branch-versus-main
capture. Keep the task `doing`; #107 and the turned-camera/corner acceptance
remain unresolved. Owner game/config/cache were not written.

### 2026-10-31 gpt controlled fly-view seam diagnostics

Done: Re-exported `klamall` and `kladwtwn` from the current main game resources into `build/scene-cache`. Captured the STOP corner (`klamall:13050`) and dark-house eaves (`kladwtwn:17104`), plus controls (`klamall:13250`, `kladwtwn:17304`), at turns 0 and 40. All snapshots passed `tools/tour/tour.py`'s `check_views` checks for map, elevation, view tile, 3D renderer, and distinct turn images. Sidecars record hero rotations 3/4; their camera string still says static preview, so that metadata is not used as turn proof.

Diagnosis: `-Shadows` changes broad regions on both targets and both controls: 54,922/2,029,191 changed pixels at the STOP view and 527,783/1,956,234 at the dark-house view. It does not isolate the reported line. A temporary `walls.bin` diagnostic setting each vertex depth anchor to its own xyz gives 0 changed pixels at all four stops and both turns, so the stored depth plane does not affect these captures. A flat-RGB wall-atlas diagnostic preserving alpha changes large regions (848,209 to 1,683,674 pixels per target frame); the atlas pages are shared with other scene sprites, so it does not isolate the seam's texel. The exact seam source remains unresolved. No production code or reference image changed.

Evidence sheets, each below 1 MB: `runs/110-corner-join-seams/diagnostics-20261031/baseline-shadows-on-vs-baseline.jpg`, `flat-wall-texture-vs-baseline.jpg`, and `depth-at-geometry-vs-baseline.jpg`. `provenance.json` records the cache variants, camera stops, and executable identity (SHA-256 `c7af38c23acc9d198e1a3e2543a75faafbd04860734e3e16910592828eecdb4f`, sidecar `ce_commit=b8eb43c5`). `ce/src` and `tools/scene_export` match current main; the captures use the branch Release executable and fresh explicit `build/scene-cache`.

Check: `ctest --test-dir build -C Release --output-on-failure` passed 18/18; `py -3 -m unittest discover -s tools/scene_export -p 'test*.py'` passed 67/67. Full `tools/regression.ps1` used its explicit `build/scene-cache`: 14/16 references passed; `klagraz-e0` differs by 183 pixels and `klaratcv-e2` by 6,299, the same stale-reference differences recorded earlier. Task 107 remains unresolved, so keep this task `doing` and PR #172 draft. Owner game/config/cache were not written.
