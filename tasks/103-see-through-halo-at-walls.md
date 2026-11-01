---
id: 103
title: The see-through halo round the hero works at walls again
milestone: M1
status: doing
owner: glm
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

- The owner, 2026-11-01, live on main `e434c327`, found the exact spot:
  Klamath downtown, the hero in the lane between the low building and the
  bathhouse, behind the low building's wall. In 3D the wall is not cut and
  the hero is drawn over it; the original view cuts the halo.
  `runs/inbox/2026-11-01-halo-3d.webp` | `runs/inbox/2026-11-01-halo-original.webp`.
  The save of that moment (read only, copy it before loading):
  `game/user-assets/saves/2026-11-01-103-halo-slot01/` (slot 01, "123").

## Acceptance

- Name the commit that lost it (git bisect with a dev snapshot of that
  bathhouse spot) and fix the cause.
- before | after of the bathhouse spot and of one more wall, in
  `runs/103-see-through-halo-at-walls/`; the roof case unchanged.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

The halo's shape and size.

## Log

### 2026-11-01 glm: the cause found and fixed

Done: the commit that lost it is c05a0f34 (082 + 062, the sun shadows):
its shadow-map dilate pass borrows texture unit 1 for the map's id
texture and leaves the unit empty. bindEgg had bound the egg mask on
unit 1 for the whole scene draw, so the wall pass right after the shadow
pass sampled the egg share from an empty unit, got 0, and discarded
nothing: the walls stood solid round the hero, and the hero card drew
over them or vanished behind them. The roofs kept their cut because
their pass re-binds the egg mask itself (roofs.cc, drawRoofs). The
regression never caught it: the test config runs with shadows=0, so the
dilate never ran there.

The fix: the dilate pass saves the unit's 2D binding and puts it back
(shadow.cc, bindStaticShadows). One general rule, no per-object case.

Evidence (runs/103-see-through-halo-at-walls/, both shots with the sun
up and the shadows on, the test fixture kladwtwn, the camera in the lane
by the bathhouse):
- bathhouse-spot-before | after: the hero behind the bathhouse's south
  wall; before, the wall stands solid and the hero is hidden; after, the
  egg cuts the wall round him.
- second-wall-before | after: one more wall section of the same
  building; before, the hero is gone behind the wall; after, he shows
  through the cut.
- roof-case-unchanged: the roof halo before the fix (it never broke).

Checks: ctest 18/18; the regression 16/16 spots with 0 changed pixels
(the fix touches the shadow pass's texture state only, and the test
config runs shadows off); the A/B on the same exe source with -Shadows
on and off brackets the cause.

Next: the owner checks it in the game with their save.
Check: the crops above; the fixture command in the Log of the PR.
