---
id: 057
title: See-through holes in fitted solids get patched over
milestone: M1
status: todo
owner: none
suggested: claude
claude-only: no
user-check: yes
depends-on: [037, 039]
reviewed: no
---

## Goal

Fitted solids (box, box+back, drum) stop showing the scene through
themselves: wherever the sprite silhouette does not cover the fitted
volume, a patch fills the hole, so junk piles and stoves read as whole
things from turned views.

## Context

- The owner's request of 2026-10-23: research how to glue the holes over;
  "the barrels and the dam are almost fine, if the holes in the models
  were patched with something". Evidence: runs/057-solid-holes/
  owner-holes-patch.png (njunk2 fans apart with a hole in its middle,
  njunk5 and njunk6 show the ground through the heap, sstove2 is close),
  and the same sheets in runs/039-shapes-klamath/.
- Candidate directions to weigh (the design doc's "one algorithm" holds:
  no per-thing hand patching):
  - an inside liner: draw every fitted solid's far sides always, in a
    darkened tint, so a hole shows a dark interior instead of the world;
  - a shrink-wrap shell: a low-poly mesh from the sprite's alpha outline
    (a height field over the footprint) drawn behind the fitted faces;
  - alpha-dilated filler quads along the silhouette edges of the fitted
    faces, so near-silhouette holes close with stretched sprite texels.
- The patch must not change the game's own view (the fitted faces there
  stay pixel for pixel; a liner only shows where a hole would be).

## Acceptance

- njunk2, njunk5, njunk6 and sstove2 read as whole things from 90, 180 and
  270 degrees in new sheets; no scene bleeds through their middles.
- The eval score does not drop; the game's own view unchanged.

## Log
