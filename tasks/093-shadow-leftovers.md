---
id: 093
title: Shadow leftovers on corners, an eave strip and a table poking through a wall
milestone: M1
status: done
owner: none
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

The six shadow faults the owner found in the live game after 090 are gone:
a building's corner takes the same shadow as the walls beside it, at every
hour.

## Context

- 090 (PR #126) closed the seams; the owner, 2026-10-27, played it live:
  "already very good, no gaps", and found these, shots in `runs/inbox/`:
  1. `2026-10-27-shadow-leftover-1.png`, `-3.png`: corners of a brick
     building partly ignore the shadow; the corner piece stays lit while
     the walls beside it are shaded.
  2. `-2.png`: a strip on the building's corner (red outline), at the top
     of the corner under the roof's edge.
  3. When the sun moves to the other side, the corner goes dark as a whole.
  4. `-5.png`: the edge of the stuff on a table sticks out through the
     bathhouse wall (in the shot, the boxes and planks by the wall's
     corner).
  5. `-4.png`: the shadow moved onto the left house's corner; the owner
     guesses the corner element is one object, so it is shaded as a whole.
  6. `-6.webp`: the same on the cow pen's fence: a corner post is one
     piece, so the shadow darkens all of it.
- Faults 1, 3, 5 and 6 look like one cause: a corner is one card or one
  object, so it gets one light value for both faces, or the face of the
  wrong wall. 090 already gives caps and door cards their own wall's side
  of the sun; check how a corner piece is lit against that.
- Find each spot with the fly camera and the dev panel's hour; the 090
  measurement (`tools/` seams metric, the 16 views and 3 fly views)
  stays the yardstick.

## Acceptance

- Before | after in `runs/093-shadow-leftovers/` for each of the six, at
  morning, noon and evening, on the same frames.
- The 090 seam totals do not rise on any view.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

New shadow features, the moon, shadow softness.

## Log

### 2026-10-27 glm: the fly fix lands; the corner class needs the owner's rule (moved to 096)

- The devshot fly camera raced its own takeoff: circleFlight ran on the same
  frame the fly axes were still the game camera's, so `-FlyTurn` pictures
  stayed at the game's offset or landed black. The takeoff now waits up to
  30 frames for the axes to settle (`devtools.cc`). Note for picture checks:
  in preview mode the JSON's camera line always reports the fixed preview
  camera, fly or not; judge a fly shot by the picture.
- The six leftovers were hunted with before | after sheets at hours 8, 13
  and 17 (`runs/093-shadow-leftovers/`, `before` on main's build, `after`
  on this branch's final code; at every framing the final pair diffs to
  0 px, so the sheets also prove nothing regressed) and the diagnosis views
  in `runs/093-probe/`. Fault by fault:
  - 1, 3 (the brick corner lit or dark as a whole), 5 (the painted post by
    the bathhouse) and 6 (the pen's corner post) are one class. ShadowDebug
    2 proves the corner piece takes the crossing wall's side of the sun
    since 090's painted-plane rule: at hour 8 the Buckner pillar reads
    shaded beside its lit face, at hour 17 lit beside its shaded face
    (`runs/093-probe/f1-debug-old-vs-new.png`), on main's code.
  - The exporter split (cut the corner picture at the crossing wall, carry
    the stretch onto the crossing plane) was tried at three cut planes. At
    the Buckner corner the rows end inside the crossing band, so nothing
    moves there; where the cut bites, the 090 seam metric rises at the
    evening views (19265 h17 133 to 213 with the nearest-plane cut, 233
    with the far-face cut). Reverted: the exporter is back at main's, the
    scene fingerprint is main's again.
  - 2 (the strip at the top of the corner under the roof edge) is the same
    piece class at the roof line; it moves with the same lever and the same
    evening seams followed it.
  - 4 (the boxes and planks by the bathhouse's corner): the boxes are
    painted into the wall picture, not scene objects (nothing at that spot
    in `scene.json`). At every framing built here (the game view at hours
    8, 13, 17, the roof-culled interior view, the alley fly views) the 3D
    matches the vanilla framebuffer shot for shot
    (`runs/093-probe/f4-corner-vanilla-vs-3d.png`). The owner's exact
    scroll reproduces it; the owner's view tile with the dev panel open
    would pin the delta.
  - A per-card shader rule (`sunShadowStanding`) was tried for 5 and 6:
    fences and painted posts are not standing cards (the pen diff is 0 px
    at all three hours) while wide card clusters did move; reverted.
- The corner class moved to 096 with the evidence references; the owner
  set its direction on this PR: the pieces are cut at the corner line,
  each half on its own face's side of the sun. Leftover 4 is not the
  corner class: the live poke-through by the bathhouse is a scene object
  off its table, task 100. The task closes here; the six shots re-check
  by eye in 096 and 100.
- ctest 18/18; the corrals regression 0 px; with the split out, the 090
  seam totals sit at main's on every measured view.

### 2026-10-27 glm: closed on the owner's review of the PR

- The owner reviewed the branch and set 096's direction: the corner
  pieces are baked as two pieces cut at the corner line, each keeping its
  own face's plane, and the 090 seam totals must not rise. Status is
  done; the corner work continues in 096, leftover 4 in task 100.
