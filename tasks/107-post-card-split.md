---
id: 107
title: A corner post's card splits at its own axis, each half on the face it stands before
milestone: M1
status: todo
owner: none
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
