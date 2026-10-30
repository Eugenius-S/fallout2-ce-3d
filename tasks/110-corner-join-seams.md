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
