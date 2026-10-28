---
id: 109
title: A standing card and a corner piece take the sun shadow where it falls on them, not all or nothing
milestone: M1
status: doing
owner: glm
suggested: glm
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A standing object card (a trough, a sign, a barrel) and a wall's corner
piece darken only where a cast shadow actually reaches them. A card that
no shadow touches stays lit; a card the shadow edge crosses is dark on one
side of the edge and lit on the other, like the floor and the walls.

## Context

The owner, 2026-10-28, live on main `9705aca4`, Klamath:

- `runs/inbox/2026-10-28-trough-shaded-whole.png`: the trough in the pen
  is dark as a whole, though no shadow lies on it; the fence's shadow ends
  at its foot.
- `runs/inbox/2026-10-28-sign-shaded-whole.png`: the sandwich-board sign
  on the street, the same: dark whole, no shadow reaching it.
- `runs/inbox/2026-10-28-corner-shaded-whole-a.png` and
  `-b.png`: corner pieces (the striped post of a dark house, the corner of
  the "1005" shack) are shaded whole. The owner: take the corner pieces out
  of the "they are sprites" rule, a corner must be able to carry a partial
  shadow.

Where it comes from (`ce/src/scene_renderer/scene_shader.cc`):

- `sunShadowAtAnchor`: a standing card takes its feet's majority, in or
  out, one verdict for the whole card (090, made for doors and signs that
  stand in their wall's line). The feet disk sits on the map, so a card
  whose foot touches a neighbour's shadow (the fence by the trough, the
  curb by the sign) goes dark from top to bottom.
- `cardTurned`: a card standing in a wall's plane, on a wall turned from
  the sun, takes the flat shaded value whole. The corner pieces read as
  such cards.

## Direction

One rule for every standing card: each pixel reads the shadow at its own
place and height on the card's plane (063 did this before 090 moved to the
feet majority), so the shadow edge crosses the card where it crosses the
ground behind it. Keep what 090 fixed: a door or a sign in its wall's
line must not come out a fifth or a half dark against its wall (the noon
door at 19265), and a walker still darkens within one game pixel of
travel. A corner piece follows the plane of the face it continues (096's
cut), so each half reads its own face's sun side and the cast shadow on
it, not one flat value. If the people (critters) read worse with per-pixel
shadow, they may keep the feet verdict; name that split in the Log and the
reason, by the object's kind in the engine's data, not by name.

## Acceptance

- before | after in `runs/109-cards-take-shadow-where-it-falls/` at the
  four owner's spots above (hours 8, 12 and 17) and at the 090 door spot
  (19265), plus one more map: the trough and the sign lit where no shadow
  falls, a shadow edge crossing a card when one does, the corner pieces
  with partial shadow.
- The 090 seam totals do not rise; the fly tour against main lists which
  spots change and why; the original view regression stays pixel for
  pixel; ctest passes.

## Out of scope

Seams at corner joins (110), the corner post split (107), the shape of
shadows themselves.

## Log
