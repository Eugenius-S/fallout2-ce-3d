---
id: 122
title: Things turn in flight: objects with full rotation sets face the fly camera
milestone: M1
status: todo
owner: glm
suggested: owner
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The owner, 2026-10-31: 087 made critters turn with the fly camera; now every
OBJECT whose art carries all six rotations and more than one of them turns
the same way. An object the fly camera looks at from another quarter shows
the rotation frame for that quarter instead of staying at its made rotation.

## Context

- Map objects carry their rotation; the engine draws scenery as live cards
  (boards) or baked solids. The turn applies to the cards; a stood solid
  keeps the projection it was made from (its art's rotation at export time).
- Rule, not name lists: an art qualifies when it has all 6 rotations and
  more than one of them exists; anything else keeps its card as made.
- Exclusions (the owner's list): unique one-off objects such as the vault
  dweller head in Arroyo and the fallen vertibird stay as they are.

## Acceptance

- Fly pairs of multi-rotation things (a table, a bookcase, a locker bank)
  from two opposite quarters: the art's facing follows the camera.
- Unique objects and single-rotation arts do not turn.
- The game's views stay pixel for pixel; ctest passes.

## Out of scope

Critters (087 did them), solids re-projecting per quarter, unique objects.

## Log

### 2026-10-31 glm: opened from the owner's order

Queued behind 120 and 121. Engine side first (where the fly camera picks a
card's frame), then the exporter's exclusions if any art lies about its set.
