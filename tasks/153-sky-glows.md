---
id: 153
title: The sky does not glow or bleed onto the map in flight
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

In the fly camera the sky keeps 142's dusty, muted colour and stays a
background: no bright saturated yellow, and no orange halo bleeding from
the sky over the map's edges and roofs.

## Context

- The owner, 2026-11-02, live on main `3bd28ea5` (just after 142, #204),
  Klamath in flight: `runs/inbox/2026-11-02-sky-glows.webp`, "the sky has
  started to glow in the 3D view". The sky reads bright lemon yellow, far
  from the beige in `runs/142-desert-sky/horizon-no-ridges.jpg`, and a soft
  orange glow spreads from it over the map's edges, the far roofs and the
  corn.
- The owner's game runs with the glow on (`--dev-*` captures force it off,
  so 142's evidence did not show this). Likely cause: `drawSceneSky` draws
  into the scene capture (`scene_draw.cc`, `beginSceneCapture` /
  `compositeGlow` in `post.cc`), so the glow pass takes the bright sky as an
  emitter and adds its bleed, which both brightens the sky and halos the
  map. Confirm with `-Fly -Glow` in the background run, then fix it by a
  rule (the sky emits no glow: drawn outside the glow source, or masked out
  of it), not by darkening the palette.

## Acceptance

- before | after in flight with the glow on and off, at 08:00, 12:00,
  17:30 and 23:00, in `runs/153-sky-glows/`: the sky matches 142's colour
  with the glow on, no halo on the map; glowing things (fire, signs,
  energy weapons) still glow.
- ctest passes; the regression stays pixel for pixel (the game view draws
  no sky).

## Log

- 2026-11-02 Cut from the owner's live find after #204.
