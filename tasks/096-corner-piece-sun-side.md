---
id: 096
title: Corner pieces take the crossing wall's side of the sun
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
edge crosses it per pixel like it crosses the walls beside it.

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
- Leftover 4 (the boxes and planks by the bathhouse's corner) is painted
  into the wall picture, not scene objects; at every framing 093 built the
  3D matches the vanilla framebuffer shot for shot. The owner's exact scroll
  reproduces it; his view tile would pin the delta.

## Direction (the owner decides)

The exporter knows the column of every texel of a corner piece; the engine
takes the sun side from the depth plane. The candidate: bake a corner piece
as two pieces cut at the corner line, each carrying ITS OWN face's plane
(not the crossing wall's), so each half follows the face it continues. The
090 seam acceptance (the totals do not rise) applies; 093's three cut
attempts all violated it, so the pieces must keep their own planes rather
than take the crossing wall's.

## Acceptance

- The owner's shots 1, 2, 3, 5 and 6 reproduce fixed at his hours.
- The 090 seam totals do not rise on any view.
- The regression stays 0 px on the unchanged views; the changed views come
  with updated references and their diff images.
- ctest passes.

## Out of scope

New shadow features, the moon, shadow softness.

## Log
