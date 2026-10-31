---
id: 120
title: Scenery stands in 3D everywhere: drums trimmed at walls, no solid loses a face
milestone: M1
status: doing
owner: glm
suggested: owner
priority: high
claude-only: no
user-check: yes
depends-on: [119]
reviewed: no
---

## Goal

The owner, 2026-10-31, on the 119 pairs: accepted overall, and while the
reviewer is away the 3D-ification gets worked through the whole typical
scenery. Nothing pokes through the walls behind it; every face of every
solid keeps its texels; the stands read as the things they stand for from
every fly quarter.

## Context (measured 2026-10-31, in-process survey)

- Fly-only drums (the 119 stands) skip the wall cut in scene.keep_apart, so
  a drum crosses every wall slab its plan circle touches: the bathhouse
  stove (klamall 24138, r=0.402) crosses the plank fence slab (u 68.97..69.02),
  the east stove (kladwtwn 26106, r=0.480) crosses its building slab
  (v 64.86..65.08) by 0.56 units, the downtown and warehouse stoves likewise.
  The owner: trim the backs so they stop poking through.
- The old shrink cut stumps a drum whose center sits on or inside the slab
  band: it kept at most 2*MIN_DRUM (0.2 units) of a 0.9+ diameter. It also
  picked the kept side from the row's anchor (row[1+axis]) while the fit may
  put the drum's center half a unit away (bathhouse: anchor u 68.958, drum
  center u 69.367), so it could keep the wrong side even when it did not
  stump.
- Boxes lose faces to solids.clip when a face's texels leave the sprite
  rectangle. Measured on the mall lockers (klamall 15683, z1=1.449, the art
  top at the anchor is z 1.567): prop 31's top-face texels all fall outside
  (the whole top face is gone), prop 32's half do (a wedge remains), prop 33
  is fine. The rat-cave locker's top is fine; its complaint is the sides: a
  narrow box's near side faces sample the art's edge columns (the wooden
  frame), so from behind it reads as a bare plank pillar.
- Do NOT touch unique one-off objects (the owner's exclusion list): the
  vault dweller head in Arroyo, the fallen vertibird. The sweep works the
  typical cases only.

## Acceptance

- Fly pairs: no drum crosses a wall plane; a trimmed drum keeps its size
  and its card's place, its back ends flat at the wall face.
- No solid face vanishes or wedges: every face of every prop solid keeps
  its texels inside the sprite, or the fit is fixed so they do.
- The lockers read as lockers from every fly quarter (fresh pairs).
- The sweep over the maps stands nothing new that reads worse than its
  card; the game's views stay pixel for pixel (16/16 regression spots,
  0 changed pixels); ctest and the exporter unittests pass.

## Out of scope

Things turning in flight (a separate task), unique objects, hole filling
(separate task), engine work unless a slice needs it.

## Log

### 2026-10-31 glm: opened with the measured drum and box fixes

Branch note: branched from task/119-lid-drums-render (119 not merged yet),
per the board's depends-on: the fly-only stands this task fixes are 119's
machinery. The design for the drum trim: scene.keep_apart annotates a
fly-only drum row with the wall planes it crosses (the slab face nearest the
drum's center, keeping the center's side) instead of skipping or shrinking;
solids.solid_faces clips the drum's side quads and its top cap at those
planes before texel assignment, so the mesh keeps its size and ends flat at
the wall. Seen=0 drums keep the established shrink behavior untouched.
