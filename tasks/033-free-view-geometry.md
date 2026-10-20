---
id: 033
title: Things seen from anywhere by the fly camera
milestone: M1
status: done
owner: claude
suggested: claude
claude-only: no
user-check: yes
depends-on: [031]
reviewed: no
---

## Goal

Under the fly camera things read as things from any side, as in the donor's
first person: sprites face the camera, solids are whole. This is also the
geometry the sun's shadows (021) are cast from.

## Context

- The owner's report (2026-10-20, fly camera in a Klamath room): a chair and
  a stove are flat cards that lean through the bed when seen from the side;
  the donor's viewer showed such things better.
- Live objects and the hero are cards built every frame
  (`scene_renderer/live_objects.cc`, `standingSpriteCard`): upright planes
  along the game's screen, leaned so the game's camera sees the sprite.
  The donor stands a card upright and turns it to the viewer
  (`vendor/fp/things.py`, "card ... faces the viewer").
- Solids (`tools/scene_export/solids.py`) export only the faces the game's
  camera sees: a drum's near half and top, a box's top and two near sides.
  The donor gives the far sides too, mirrored from the near ones
  (`solid_faces` with board -1 already does it for drums).

## Acceptance

- While the fly camera flies, every standing card (live objects, the hero)
  stands upright on the middle of its footprint and faces the camera; the
  game's camera sees exactly what it saw (regression 16/16, no reference
  changes).
- While it flies, solids show their far sides (mirrored, as the donor does);
  the game's camera does not draw them.
- Snapshots from the air before and after in `runs/033-free-view-geometry/`.

## Out of scope

Low piles laid flat, sheets along walls (`things.py` rules 2 and 3), walls as
slabs: a later task if the owner wants them. Static rest cards (a drum's
flames, stems) stay as they are.

## Log

### 2026-10-20 claude
Done: under the fly camera every standing card (live objects, the hero)
stands upright on the middle of its footprint and faces the camera
(`faceFlyCamera`). Solids get their far sides: a box's two, a drum's far
half, showing the near sides' pixels mirrored and wound to face away from the
game's camera, which culls them (no sidecar format change). Regression 16/16
pixel for pixel on the new export: the game's camera sees nothing new.
Evidence in `runs/033-free-view-geometry/`: the Klamath room with the bed from
above and from 70 degrees round (before | after), from 140 and 180 degrees
after. New dev options to take such views: `-FlyTurn`, `-FlyPitch`,
`-FlyDistance`.
Seen and left: walls are still single sheets, so from behind a wall its
picture's top rows lie on its top edge (walls as slabs, out of scope).
Next: merge, export Klamath into `game/mod-cache`, the owner's look from the
air.
Check: `py -m unittest discover -s tools/scene_export`, `tools/regression.ps1`,
`tools/dev-snapshot.ps1 -HeroTile 17665 -ViewTile 17467 -FlyTurn 70 -FlyPitch 25 -FlyDistance 7`.
