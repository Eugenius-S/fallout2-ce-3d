---
id: 111
title: The slime pools glow and the lamps light the walls in the live caves
milestone: M1
status: doing
owner: glm
suggested: glm
priority: normal
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

### 2026-11-01 glm: the live-game side investigated, the cause narrowed to two items

Done (no code changed; the read-only inspection of the owner's game):
- The owner's mod-cache scene folders: klatoxcv 0/1/2 are COMPLETE (all 12
  files each). kladwtwn holds elevation 0 ONLY; artemple and vault15 have
  no folders at all. The owner's game\logs\scene.log tail agrees: artemple,
  vault15 and kladwtwn elevation 1 each log "stays 2D: floor.bin is missing
  or unreadable" - those maps are correctly 2D for want of a cache.
- klatoxcv has NO lines in the owner's scene.log: the loader never failed
  there, so the live 2D in the caves was the 3D view being off - per the
  owner's 11-01 note, the hero was in combat, and the 3D view stops in
  combat (135). Out of combat the caves are 3D: the sweep's fly tour
  (build/tour/after/klatoxcv-e0..e2 in the glm worktree) shows all three
  elevations in 3D.
- The glow round the slime: every snapshot fixture runs with the glow
  forced OFF (dev-snapshot keeps captures byte-stable; -Glow keeps it on),
  so the tour stills show the pools flat by design and prove nothing about
  the live look.

Next: one live-like shot - dev-snapshot -Map klatoxcv -Elevation 1
-ViewTile 19091 -Glow WITHOUT -Still (the palette cycles, the glow on) -
and judge the slime's halo and one lit wall; if they work there, close
this task on that (the task's own 11-01 direction). kladwtwn/1 and the
arroyo maps on the owner's side ride with 136's group export and an
owner-side re-export.

Check: game/logs/scene.log tail; the mod-cache listings above.
