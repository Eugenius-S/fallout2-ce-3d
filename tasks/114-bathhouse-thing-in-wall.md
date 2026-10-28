---
id: 114
title: The thing in the bathhouse wall: find what it is and put it where the game has it
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

The grey thing sticking out of the bathhouse's alley-side wall is gone from
the wall in every camera, and whatever it is stands where the original game
puts it.

## Context

- 100 (merged, #147) decided the thing was two `Junk` scenery piles in the
  alley (hexes 16478, 16479) and turned the mapper's loose clutter back
  into cards. The owner, 2026-10-28, live on main `ff5f4ed0` (with 100):
  the thing is still there, and it is not alley junk.
  - `runs/inbox/2026-10-28-bathhouse-wall-thing-iso.png`: the 3D view at
    the game's angle, the grey thing on the wall left of the hero, by the
    corner near the door (also `2026-10-28-bathhouse-corner-piece.png`).
  - `runs/inbox/2026-10-28-bathhouse-wall-thing-top.webp`: flight, roof
    off. Outside the alley wall lies a grey pipe; inside against the same
    wall stand a box, a plate and a can; the table with its things is on
    the right. The owner: the things on the table moved off it and stuck
    in the wall, physically moved.
- So 100 named the wrong objects. Start from the objects, not a theory:
  list every object (pid, hex, elevation, FRM, flags, the art's own x/y
  offset) whose picture lands on that stretch of wall, inside and outside,
  and the table's things; compare where each stands in the original view
  at the same scroll and in the 3D scene.
- Suspects, none proven: the frame offset of the art (FRM shift) taken or
  dropped twice; a lift (things standing on tables) that resolves to the
  wrong table or none; a card anchored at the wrong hex.

## Acceptance

- The Log names each object and the line that put it in the wall.
- before | after with the original at the same scroll, the 3D at the
  game's angle and in flight with the roof off, inside and outside, in
  `runs/114-bathhouse-thing-in-wall/`; the other tables of Klamath with
  things on them checked the same way.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

The shapes of the things themselves (115, 116).

## Log
