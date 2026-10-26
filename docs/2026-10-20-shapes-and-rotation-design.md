# Shapes of things and camera rotation

Agreed with the project owner on 2026-10-20.

## Goals, in order

1. The game's view stays exactly the 2D game, pixel for pixel. Nothing below
   may change it; the regression guards it.
2. Camera rotation is an option, off by default: `[3d] camera_rotation`, in the
   menu as "Camera rotation". Updated by the owner on 2026-10-25 (077): it turns
   in 45-degree steps around the vertical axis at the game camera's pitch,
   limited to -45..+45 degrees. `[3d] camera_rotation_unlimited`, off by
   default and marked "not recommended", lifts the limit. The dev fly camera stays
   unlimited; view quality beyond the limit is not promised.
3. Seen from a turned camera, things look decent by an algorithm, not by
   fixing chairs one by one: at least 70% of the eval set and at least 70% of
   Klamath's static things (weighted by how often they stand on the maps)
   pass. Some hand-written exceptions are allowed, within a cap.
4. Sun shadows come from buildings (walls, roofs, doors) and big things only.

A thing's back is its front mirrored: there is one picture per thing. Seen
from 180 degrees a bed or a locker shows its front reversed. That is accepted;
real backs need models (M2).

## Shapes

Every static thing gets one shape. Each shape is textured by projecting the
thing's sprite from the game camera, so the game's view sees the same pixels
whatever the shape is; the other sides show the near sides mirrored.

| Shape | For | Today |
|---|---|---|
| Card | critters, anything the algorithm is unsure of; stands upright on its footprint's middle and faces the camera | live objects under the fly camera (033) |
| Flat | low piles, junk, goo | donor decals (035) |
| Plane on an axis | doors, gates, lockers' doors, anything that opens or animates; lies on its wall's or its own axis, never turns to the camera | none, doors are cards |
| Sheet | boards, shelves, counters: a thin slab with its top and ends | walls only (034) |
| Crossed sheets | plants | donor viewer only |
| Box | crates, stoves, computer stands, tables | donor box fit |
| Drum | barrels, hydrants, round tables | donor drum fit (028) |
| Box with a back | chairs, beds: a box for the seat, an upright plane on its back edge for what rises above it | none |
| Several boxes | heaps of crates, things the artists cut into several objects | donor `groups`/`compose`, partly |

Critters stay cards and show the one of their six directions nearest to how
the camera turned.

## The algorithm

Inputs per thing: the sprite's alpha, the proto's type and flags, its
footprint on the hexes, its frame count, the walls next to it, and what the
donor decided (`vendor/fp/things.py` `stand`, `props.py` fit and groups).

Rules, first match wins:

1. Animated or opening (doors, gates, lockers): plane on the axis of the wall
   it stands in, else on its own longest edge.
2. Flat by the game's flag or by the donor: flat.
3. Plant: crossed sheets.
4. Thin along an axis: sheet.
5. Fits a box or a drum from the silhouette (its bottom edges give the
   footprint, its top face the height): box or drum. If the part above the
   fitted top is a thin upright strip along one edge: box with a back.
6. Several fits side by side: several boxes.
7. Otherwise, or when a fit's confidence is low: card. A card is never worse
   than today, so doubt always falls back to it.

Exceptions: `tools/scene_export/shapes/overrides.json`, the FRM name to a
shape and its numbers, each with a one-line reason. At most 10% of Klamath's
distinct static FRMs.

## The eval set

About 40 Klamath FRMs: the 20 that stand most often (they cover 94% of the
boards on the maps) and 20 chosen for their kind and for the owner's reports
of 2026-10-20 (`runs/035-donor-things/owner-*`): the notice board, the
hydrant, the round table, doors and a fence gate, the stove, the bed, chairs,
crates, the computer stand, a barrel, a locker, car parts.

- Each FRM has a label in `tools/shapes/eval/labels.json`: the expected shape
  and, for solids, its rough size in hexes and height. Claude wrote them
  (task 036); the owner changes them at will. No agent changes a
  label, least of all to make it pass.
- Each FRM has a contact sheet: one instance on its map seen from 0, 90, 180
  and 270 degrees (and 45 for the 45-degree question), at the game camera's
  pitch, side by side with its label.
- An FRM passes when its shape is the labelled one and its sheet shows no hole,
  no fanned planes and no stray part. The first is checked by a script, the
  second by eye.
- The score is the share that passes. It is written into the task's `Log` on
  every run.

## Rotated views

- The camera turns over about a quarter of a second; the game's view (0
  degrees) is the default. [ and ] turn it a quarter each way
  (`scene::turnCamera`); four turns one way bring the game's view back. A
  turned camera stands where a plain perspective camera shows the same width
  of floor as the game's view, and draws as the fly camera does: no interface.
- Version 1 is look-only: clicks and the hex cursor work at 0 degrees, a turned
  view only looks. The game picks objects in its own 2D screen, which a turned
  view does not match; picking in turned views is a later task if wanted.
- The see-through walls (018) and the hidden roofs (013) follow the camera's
  direction: the walls between the camera and the hero are the ones cut.
- The fly camera (031) stays as it is.

## Shadows

Casters: walls with their backs and ends, roofs, doors, and things whose
shape is a box, a drum, several boxes or a box with a back, at least two hexes
across or taller than a wall's half. The other things cast no sun shadow and
keep the shadows painted into their art. Task 021 follows this.

## Stages

1. 036: the eval set, its labels and contact sheets, the score of today's
   shapes. Done.
2. 037: the algorithm, until the eval score is 70% or more. Done.
3. 038: the rotation option, look-only, 90-degree steps. Done.
4. 039: all Klamath: every static FRM's sheet looked at once, exceptions for
   the frequent failures, the weighted Klamath score 70% or more, the owner's
   look from turned views.
5. 040: the eval FRMs still failing (any agent, any time).
6. 043, 044: the eval shows only changed sheets and finds vanishing things by
   itself, so fewer sheets need eyes.
7. 048: labels from a second town (the Den), so the score is not a fit to
   Klamath.

Where it stands (2026-10-20): the eval is 35/40 (88%), weighted 99%.

Stop points: if a change leaves the score lower twice in a row, stop, write
the score and the failing FRMs into the `Log`, and take other work. Klamath
first; the rest of the game is a later decision.

## How the agents work on this

[AGENTS.md](../AGENTS.md) holds; on top of it:

- An agent may decide alone: the algorithm's inside, its thresholds, the
  tools, the code layout.
- An agent may not: change the eval labels, lower the 70% gate, change the
  game's view, retake regression references without a before | after image,
  or add exceptions beyond the cap. Any of these goes into `tasks/inbox.md`
  for the owner; meanwhile take other work.
