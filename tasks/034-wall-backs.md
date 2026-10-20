---
id: 034
title: Walls with a back, ends and bridges
milestone: M1
status: todo
owner: none
suggested: claude
claude-only: no
user-check: yes
depends-on: [033]
reviewed: no
---

## Goal

A wall is a slab from every side: the fly camera behind it sees its back, and
the sun's shadows (021) are cast by slabs, not by single sheets.

## Context

- The owner's request (2026-10-20): all walls two-sided.
- Today `walls.bin` holds a wall's front and its top (`wall_sidecar.py`,
  `_wall_front`); seen from behind, a wall is not drawn (culled) and the top
  rows of its picture lie on its edge.
- The donor already works the rest out (`vendor/fp/boxes.py`, `thicken`): the
  back (the picture of the nearest plain piece seen from that side, windows
  kept open), the ends of free stretches and doorways, the bridges where a
  wall ends at another. Find what of it reaches `scene.json` and draw it,
  wound away from the game's camera (as 033 does for solids) so its view does
  not change.

## Acceptance

- From the air (`dev-snapshot.ps1 -FlyTurn ... -FlyPitch ... -FlyDistance ...`)
  walls of Klamath Downtown and a cave show their backs and ends; before |
  after in `runs/034-wall-backs/`.
- Regression 16/16 pixel for pixel.

## Out of scope

Roofs seen from below; interiors of the caves' rock (`rock.py`).

## Log
