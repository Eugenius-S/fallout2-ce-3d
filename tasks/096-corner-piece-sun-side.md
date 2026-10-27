---
id: 096
title: Corner pieces are cut at the corner line, each half on its own face's side of the sun
milestone: M1
status: todo
owner: none
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: [093]
reviewed: no
---

## Goal

A wall's corner piece (the pillar at a corner, the strip at the top of a
corner under the roof's edge, a fence's corner post) takes the side of the
sun that the face it visually continues takes, at every hour, and a shadow
edge crosses it per pixel like it crosses the walls beside it. A corner
piece stays where the original shows it in every camera, and no shadow
runs past it round the corner onto the next face.

## Context

- 093's investigation (its Log, the sheets in `runs/093-shadow-leftovers/`
  and the diagnosis views in `runs/093-probe/`) narrowed the owner's
  leftovers 1, 2, 3, 5 and 6 to one class: a corner piece's art spans two
  faces, and 090's painted-plane rule gives the piece ONE face's side of the
  sun. ShadowDebug 2 proves the inversion: at hour 8 the Buckner corner
  pillar reads shaded beside its lit face, at hour 17 lit beside its shaded
  face (`runs/093-probe/f1-debug-old-vs-new.png`), on main's code.
- The natural fix (093's attempt) cuts the corner's picture at the crossing
  wall and carries the stretch onto the crossing wall's plane. Measured at
  three cut planes:
  - at the Buckner corner the wall rows end inside the crossing band, so
    nothing moves there and the pillar keeps its wrong side;
  - where the cut does bite, the 090 seam metric rises at the evening views
    (19265 h17 133 to 213 at the nearest-plane cut, to 233 at the far-face
    cut; 25075 h17 and 23481 h17 rose with the mid cut): the re-homed
    stretch reads lit inside a shaded wall. Reverted; the exporter is back
    at main's.
- The pen's corner post (6) and the bathhouse's painted post (5) are not
  standing cards: a per-card shader rule changes nothing on them (the pen
  diff is 0 px at hours 8, 12 and 13), and on maps with many card clusters
  (klatoxcv) it moved walls and pools, so it was reverted too.
- Leftover 4 is not this class: the boxes by the bathhouse's corner are
  painted into the wall picture and 093's framings match the vanilla
  framebuffer shot for shot; the owner's live poke-through there is a
  scene object off its table and lives in task 100.
- The owner's later finds of 2026-10-27 (tasks/inbox.md under (096), on
  the board branch until PR #137 merges; the shots live there):
  - the bathhouse's south corner piece is gone in the turned perspective
    camera but in place in the isometric one
    (`runs/inbox/2026-10-27-bathhouse-corner-piece-gone.jpg`);
  - the shadow flows round the corner past a lit corner piece, also at
    Vic's house (`runs/inbox/2026-10-27-vic-house-shadow-round-corner.jpg`);
  - a shadow flaw by the house in the bottom-edge shot
    (`runs/inbox/2026-10-27-bottom-edge-exit-zone.jpg`; the owner did not
    mark the spot, compare it with the original view).

## Direction

The owner set the rule (2026-10-27, on the 093 PR): bake a corner piece as
two pieces cut at the corner line, each carrying its own face's plane, so
each half follows the face it continues. Not the crossing wall's plane for
the whole piece, which is what 090 does and what inverts the piece against
its own face at the ±hours. The 090 seam acceptance (the totals do not
rise) applies; 093's three cut attempts all violated it by re-homing the
stretch onto the crossing wall's plane, so the halves keep their own
planes and their place in the picture.

## Acceptance

- The owner's shots 1, 2, 3, 5 and 6 reproduce fixed at the owner's
  hours, and the later finds with them: the corner piece back in the
  turned camera, no shadow round the corner at either house, the
  bottom-edge flaw gone.
- The 090 seam totals do not rise on any view.
- The regression stays 0 px on the unchanged views; the changed views come
  with updated references and their diff images.
- ctest passes.

## Out of scope

New shadow features, the moon, shadow softness.

## Log

### 2026-10-27 glm: the owner set the direction

- On the 093 PR the owner fixed the rule: two pieces cut at the corner
  line, each on its own face's side of the sun; the 090 seam totals must
  not rise. Leftover 4 moved to task 100. The owner's later corner finds
  from the live run (the inbox (096) entries) joined the scope.
