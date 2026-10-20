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

### 2026-10-20 claude
Done: `walls.bin` version 4 gives every batch a flag; a page has up to two
batches, the second for the fly camera only (the engine still reads version
3, and a page's two batches share one texture). Into the fly-only batches go
the backs of the solid walls (donor `wallRest`: the same outline `thick`
behind the front, up to the cap line, in the picture of the side it faces,
mirrored) and the donor's trims (ends of free stretches and doorways, bridges
where walls meet). Cut-open upper parts (`above`) are left out: Klamath has
none. Regression 16/16 pixel for pixel; exporter tests 39/39 (the first-person
test now checks the game camera's batches stay the same and the trim lands in
a fly-only one); unit tests 10/10.
Evidence in `runs/034-wall-backs/`: the house with the bed from behind, before
| after; another angle; a cave (KLARATCV e1) from the air.
Next: merge after PR #35, export Klamath into `game/mod-cache`.
Check: `py -m unittest discover -s tools/scene_export`, `tools/regression.ps1`,
`tools/dev-snapshot.ps1 -HeroTile 17665 -ViewTile 17467 -FlyTurn 180 -FlyPitch 30 -FlyDistance 7`.
