---
id: 115
title: No holes in the 3D things: transparent texels take the colour of the edge
milestone: M1
status: todo
owner: none
suggested: glm
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The 3D solids (tubs, junk, furniture) and wall faces show no holes through
to the scene behind, in the game's angle and in flight. Where the art is
transparent on a face of a solid, the face is filled in the colour of the
nearest opaque pixels.

## Context

- The owner, 2026-10-28, live on main `ff5f4ed0`:
  - `runs/inbox/2026-10-28-tub-holes-iso.png` and `-tub-holes-fly.png`:
    the bathtub by the cow pen shows the ground through itself, even at
    the game's own angle.
  - `runs/inbox/2026-10-28-inside-holes.webp`: holes inside a building
    (also the walls-from-inside work of 102, PR #150).
- The owner's rule: extend every texture so nothing has holes; fill a
  transparent texel of a face by the colour of the last opaque pixel, an
  average of its neighbours (edge padding / dilation), not by a flat tint.
- History: 061 asked for the same patch and was folded into 075; the holes
  are still there. Read what 075 and 061 tried before choosing.
- Where a cut-out is the shape itself (the open space between a tub's legs
  seen at the game's angle), the game view keeps the original's pixels:
  the fill closes faces the 3D builder made, not the original's outline in
  the game camera.

## Acceptance

- before | after of the tub (game angle and flight), two junk piles and
  one interior wall in `runs/115-no-holes-in-3d-things/`.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

New shapes for things (116), wall backs (102).

## Log
