---
id: 121
title: No holes anywhere: the edge-colour fill runs over every map and thing
milestone: M1
status: todo
owner: glm
suggested: owner
priority: normal
claude-only: no
user-check: yes
depends-on: [115]
reviewed: no
---

## Goal

The owner, 2026-10-31: work the hole filling through completely while the
reviewer is away. 115 taught props.fill_holes (an enclosed transparent
texel takes the colour of the nearest filled edge pixel) for the exporter's
things; the pass must cover everything: solids and their rests, boards,
wall sheets, forms, every map of the game.

## Context

- 115's fill runs where props build their sprites; anything the pipeline
  draws by another path (boards' live frames, shadow sprites, wall sheets,
  rock, tiles) was not audited.
- The owner's words for the fill rule: the colour of the last pixel (the
  nearest filled edge texel), which is what 115 shipped.

## Acceptance

- A survey over all maps lists every art that still shows a see-through
  hole after the fill, and the fill covers them (or the survey shows the
  hole is a real gap the game's own view also shows).
- The 16 regression spots stay at 0 changed pixels; ctest and the exporter
  unittests pass.
- Fresh before | after pairs for the holes this finds.

## Out of scope

Unique objects' arts (they keep their cards), geometry work (task 120).

## Log

### 2026-10-31 glm: opened from the owner's order

Queued behind task 120; the survey tool comes first (list what still holes),
then the fill extensions, then the pairs.
