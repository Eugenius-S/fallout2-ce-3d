---
id: 056
title: Vic's shack from the back: roof gap, open corner, hanging sign
milestone: M1
status: todo
owner: none
suggested: claude
claude-only: no
user-check: yes
depends-on: [034, 038]
reviewed: no
---

## Goal

The owner's 2026-10-20 report (task 039's Context) closes: at Vic's shack
seen from a quarter turn the roof meets the front wall, the corner left of
the VIC'S sign is closed, and the sign hangs on the wall.

## Context

- Reproduced on 2026-10-23 with the current main:
  runs/039-shapes-klamath/vic-shack-turn-270.png (and -crop.png,
  -270.jpg the owner's reference). kladwtwn, -HeroTile 17665 -ViewTile
  17467 -Turn 3.
- At 270 degrees the roof slab stops short of the front wall's top, the
  front wall's left end stands apart from the side wall (the interior is
  visible through the gap), and the VIC'S sign panel floats left of the
  rusted wall piece that carries the door.
- This is the 034 wall assembly (walls.bin geometry and how the exporter
  groups wall squares) seen from a turned view; prop shapes are not
  involved.

## Acceptance

- The three defects are gone in the turned view; the game's own view stays
  pixel for pixel (regression).

## Log
