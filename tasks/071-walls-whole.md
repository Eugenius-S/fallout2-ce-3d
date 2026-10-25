---
id: 071
title: Walls: no holes, a back side everywhere, clean corners
milestone: M1
status: done
owner: claude
suggested: claude
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

One general fix for the walls in 3D: no holes, every wall seen from behind
shows its back, and house and fence corners close cleanly. No spot patches.

## Context

From the owner (2026-10-22 and the live review of 2026-10-24):

- Some walls lost their back side: from behind in the fly camera they
  vanish. They remember every wall had one.
- Holes in walls in the 3D view. The shack's holes by the barrels may be
  fine (it is a shack), the rest are not.
- Defects at house corners; they want one universal fix.
- The fence corner at the pen is broken, and the shadows show it.

Look for a lost back face or a culling change since 034 first; 059 (panels
from behind) may share the cause.

## Acceptance

- Turned and from-behind shots of the long building, the shack and the pen
  fence under `runs/071-walls-whole/`.
- The regression stays where it is; the game's own view is untouched.

## Out of scope

Doors (072).

## Log

### 2026-10-25 claude
Done: one rule for every corner. The game draws a corner as one picture and
the donor lays it on one wall's line, so at most corners one wall ran up to
0.7 units past the other and the other stopped short (127 of 159 corners in
kladwtwn were off by more than 0.08): the free camera saw a stub and a notch
showing the street through. `_corner_fits` in
`tools/scene_export/wall_sidecar.py` finds each thick wall end that meets
another wall at a right angle (within 0.75): at an L the wall that runs on
further reaches across the other's far face and the other stops at its near
face; at a T the stem stops at the near face. An overrun is cut off for the
free camera (the row moves to the game-only batch whole and its fly copy is
clipped, its back too); a shortfall gets a block of the wall's own material
(front, back, top), and a moved outer end its end face; the donor's end
strips at moved ends are dropped. The pen's fence corners close the same way.
The shadow casters skip game-only faces (walls.cc): they cast as the free
camera sees them. walls.bin's format is unchanged.
The "missing backs" were not walls: the thin rows on the walls are signs,
posters and shelves (059 covers their backs).
Check: the exporter suite 50/50 (a new test: an L closes and the game view's
triangles stay the same), ctest 14/14. The game's view is unchanged to the
pixel: kladwtwn, klamall and klaratcv default views, old against new export,
0 pixels differ. Before and after under `runs/071-walls-whole/`
(`dev-snapshot.ps1 -FlyTurn 90 -FlyPitch 30 -FlyDistance 14`).
