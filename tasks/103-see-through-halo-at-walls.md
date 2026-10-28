---
id: 103
title: The see-through halo round the hero works at walls again
milestone: M1
status: todo
owner: none
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
