---
id: 111
title: The slime pools glow and the lamps light the walls in the live caves
milestone: M1
status: doing
owner: glm
suggested: any
priority: low
tier: 4
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

In the owner's live game, in the 3D view of the Klamath caves, the green
slime pools glow (022, 073) and the point lights light the walls near them
(099), as the snapshots show them.

## Context

- Folded in (2026-11-04 inbox sweep): the inbox line of 2026-10-24, no
  glows in the owner's live Klamath run; the burning barrels should glow.
The owner, 2026-10-28, live on main `9705aca4`:

- `runs/inbox/2026-10-28-slime-no-glow.png`: the toxic caves
  (klatoxcv), the slime pools are flat green, no glow round them.
- `runs/inbox/2026-10-28-caves-no-point-light.webp`: the toxic caves
  again; the owner noticed no dynamic light anywhere, though the caves
  look fine overall.
- The 2026-10-24 inbox line says the same of the burning barrels in
  Klamath. 073 found the snapshot fixture froze the palette cycle and
  fixed the slime there; the live game was not checked after it.

So the snapshots and the live game may differ: settings (the `[3d]`
section of the owner's ini against the snapshot's), the palette cycle in a
running game, or the scene cache the game loads.

- The owner, 2026-11-01, live on main `e434c327`: the shots of the toxic
  caves at elevations 1 and 2 (`runs/inbox/2026-11-01-toxic-caves-2d-e1.webp`,
  `runs/inbox/2026-11-01-toxic-caves-2d-e2.webp`) are the original 2D view
  because the hero was in combat, and the 3D view stops in combat (135). So
  the 10-28 finds may have been 2D too. Check the glow and the lights in 3D
  out of combat first; if they work there, close this task on that.

## Direction

Reproduce in the live game's conditions first: the owner's ini
(`game/fallout2.cfg` and the mod ini, read only, never edit the owner's
files), a non-still run, the main checkout's cache. Find which condition
turns the glow and the lights off, then fix that cause, not the look.

## Acceptance

- The cause named in the Log, with what differs between the snapshot and
  the live game.
- before | after of the slime pools and of one lit cave wall in
  `runs/111-cave-glow-and-lamps-live/`, taken in the live game's
  conditions.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

New glow kinds, the glow's look, light strength.

## Log

### 2026-11-06 glm
Claimed per the owner's order (all known 3D-ification flaws found by
glm and gpt go first). Plan: read the owner's ini [3d] against the
snapshot fixture's, then a live-run probe with the owner shot tooling
(183) on klatoxcv: the slime pool's glow state, the point light's
presence, the palette cycle in a running game. The candidate causes
are in the Context above.
