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

### 2026-10-31 glm: the cut plane settled; the price at one fence gap is on the table

Three cut-plane designs were tried against the probe views. Cutting at the
slab edge away from the drum's center kept the most drum but left the mesh
open along the cut: through a gap in the fence's art the view reached the
drum's hollow inside, and even with the cut closed by its own chord face the
strip did not return to main's look. The design that stands: cut at the slab
face the drum's center is nearest, and close the end with its chord face (the
ellipse's chord at the plane, z0 to z1, wound both ways, texels from the
sprite's centre column so the cut never samples the art's transparent edge).
The drum ends flush at the wall; nothing pokes out the far side; the fly view
shows a full-bodied drum with a flat cut end. Measured: the bathhouse stove
keeps 95% of its drum, the east stove 58%, the downtown stove 50% (those two
stand deep in their buildings' depth; the fit put them there). No
game-camera faces anywhere: the exporter unittests 75 (the sliced-drum test
checks the kept reach and the flat end), ctest 18/18.

The known price, on the table: at the bathhouse's game-angle probe (not one
of the 16 regression spots) a 14 px wide strip through the fence gap differs
from main - main shows the card's overhang hanging past the wall's plane,
which after any trim physically cannot reappear; the gap now reads as the
fence's inner side. The official 16 regression spots stay at 0 changed
pixels, and the engine's true game camera (flying=false) draws no fly-only
batch at all (scene_draw.cc gates them); the strip shows up in the still
captures because they render through the free-camera path. The owner judges
the pairs.

### 2026-10-31 glm: the drums are trimmed; the locker's light theory disproven

The drum trim landed: scene.keep_apart annotates a fly-only drum row with the
wall planes it crosses (row[16]: the slab face nearest the drum's center,
keeping the center's side), and solids.solid_faces slices the drum's side
quads and top cap at those planes before texel assignment. Measured on the
export: the bathhouse stove keeps 95% of its drum and ends flat at the
fence's far face, the east stove keeps 58% flat at its building, the
downtown stove keeps half; the game's faces are untouched (the game camera
never draws a clipped drum), so the regression stayed 16/16 at 0 changed
pixels, ctest 18/18, exporter unittests 75 (a new test: the sliced fly drum
keeps its far reach and loses exactly the part past the plane).

The locker went the other way: the "glare" theory died. The full-light mark
of 10-30 never fires for prop solids at all (their top caps land in the
both-cameras batch, so a row is never all-fly triangles), and the scene
shader's pointLights already samples the per-hex light map plus the big
lamps: the box is lit by the same model its live card is. Removing the mark
(left in place until this task, then reverted) changed nothing: three locker
fly views, 0 changed pixels. What the owner sees is the art: a live card
turns its front (and animates through frames, the open door darker) at every
azimuth, while the box shows the art's own side and back bands, which for a
locker are plain light wood, and its top face is large at a steep fly pitch.
No engine change can make the back of a frontal art look like a front; the
honest options for a later slice are fit-side (a shallower or shorter box)
or art-side, and the owner judges the pairs.

Branch note: the experiment cache is build/cache-120 (klamath group), the
pairs 119-fix | 120 in runs/120-scenery-3d-everywhere/, the shot tool
tools/shots120.ps1, the pair composer tools/pairs120.py.

### 2026-10-31 glm: opened with the measured drum and box fixes

Branch note: branched from task/119-lid-drums-render (119 not merged yet),
per the board's depends-on: the fly-only stands this task fixes are 119's
machinery. The design for the drum trim: scene.keep_apart annotates a
fly-only drum row with the wall planes it crosses (the slab face nearest the
drum's center, keeping the center's side) instead of skipping or shrinking;
solids.solid_faces clips the drum's side quads and its top cap at those
planes before texel assignment, so the mesh keeps its size and ends flat at
the wall. Seen=0 drums keep the established shrink behavior untouched.
