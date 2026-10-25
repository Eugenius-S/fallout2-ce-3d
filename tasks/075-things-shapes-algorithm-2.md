---
id: 075
title: Things in 3D: a second, serious pass on the shapes algorithm
milestone: M1
status: todo
owner: none
suggested: claude
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Furniture and other static things look like solid objects from any fly
camera angle: no torn planes, no pieces sticking through each other or
through walls, no holes. The fix is in the algorithm that turns a sprite
into a shape; where the algorithm cannot reach, the most common kinds
(bed, chair, table, stove, shelves, crates) get a shape by kind, not by
single spot.

## Context

- The owner, 2026-10-25: this needs its own serious, algorithmic task; or
  hardcode the most typical things. Their latest shots:
  `runs/inbox/2026-10-25-review2-11.png` (a small thing on the street
  folds into torn planes), `2026-10-25-review2-12.png` (a bed whose
  pillow stands up as a plane, a white box sunk into the bed, a chair and
  table beside them).
- More from the same review: a stove falls out through a house wall with
  holes around it (`2026-10-25-review2-04.png`, `-07.png`), clothes lines
  stick out over a roof (`-09.png`), the bathhouse table goes through its
  walls and the shelves read flat (`2026-10-25-bathhouse-interior.png`,
  `-bathhouse-shelves-flat.png`). First check whether the owner's build or
  cache lost solids that glm's eval shots had.
- Likely why furniture vanished: glm found on 2026-10-24 (branch
  `task/040-shapes-eval-failures`, its log) that every by-name box solid
  since 040 part 3 is a collapsed sliver: `props.fit(force=True)` fits
  around a tiny opaque core and nothing checks the box against the
  sprite's footprint, so the chair, crate1 and the car draw nothing in
  flight. Start here.
- This task takes over 040 (the eval's failing FRMs), 059 (panels and a
  box vanish from behind), 061 (holes in fitted solids) and 068 (the
  owner's 2026-10-24 spots: computer stands, the notice standee, the
  buckets, a lost card, a floor tile back to 2D) (closed into it).
- Where things stand: the design in
  [docs/2026-10-20-shapes-and-rotation-design.md](../docs/2026-10-20-shapes-and-rotation-design.md),
  fits in `tools/scene_export/solids.py` and the donor's `props.py`,
  exceptions by name in `tools/scene_export/shapes/overrides.json`, the
  eval in `tools/shapes/eval/` (35/40 at 040's end).
- Directions to weigh: a thing knows its footprint on the hex grid and
  must not leave it or cross a wall; a kind classifier (bed, table, chair,
  stove, shelf) picks a template shape; a shell from the sprite outline
  closes holes; flat details on top (pillows, blankets) stay on the top
  face instead of standing up.

## Acceptance

- The owner's spots above read as solid things from 90, 180 and 270
  degrees, before | after in `runs/075-things-shapes-algorithm-2/`.
- No thing sticks through a wall or another thing in the Klamath fly
  shots; the eval score does not drop.
- The game's own view unchanged (regression 16/16).

## Out of scope

Walls, corners and roofs (076).

## Log
