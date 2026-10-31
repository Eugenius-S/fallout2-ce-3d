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

### 2026-10-31 glm: the carve landed and verified; the light-bake leak found and closed

The carve shipped on this branch and went through the full discipline:

- The exporter unittests 78 (three new: a body-and-cabin profile splits into
  the bands, a flat profile keeps one raised block, placeholder cards are no
  directions).
- The whole-game build sweep (every map file the game ships, all their
  elevations): 268 elevations build, the only 2 failures are the
  pre-existing truncated epamain1/epamain2 third elevations (the exporter's
  own --maps flow skips them per map). Nothing new broke anywhere.
- The 16 regression spots: 16/16 at 0 changed pixels. The FIRST run failed
  7 spots (41 to 230 px) and the leak was real: a carved board goes
  fly_hidden, and the bake drops hidden boards' painted shadows
  (scene.py's shadows filter), so the light map changed around the carved
  things. The fix puts carved boards into fly_shadows and fly_sprites
  exactly like the lid-drum branch - a thing main kept a card keeps its
  painted shadow in the bake. The rerun after the fix is the clean 16/16.
- The pairs: runs/120-scenery-3d-everywhere/car4b-turn{0,90,180,270}-pair.jpg
  and car3b-... (the Den's cars, before = the card as the game had it,
  after = the carved fly-only solid) plus the close-up set at a steeper
  orbit (car4b-close*, car3b-close*). The solid follows its art closely at
  the game pitch - the volume shows at the steeper angles.

On the Den the rule stood CAR4b (two boxes: the body 0.77 tall and a second
band 0.55), CAR3b (three bands up to 0.99) and the car3 wreck piles (two-
and three-band rows, six rows on one tile). The qualification found them by
the art alone: nothing was named.

### 2026-10-31 glm: the carve measured, the art measured, the design landed on per-face sources

The visual-hull carve (six silhouettes, hex-rotation cameras, voxel
intersection) was built and run on the real arts before any pipeline work.
What the arts themselves turned out to hold killed it, with evidence:

- bath01/bath02's direction slots 3-5 hold literal "UNUSED ART" placeholder
  cards (a 46x22 full rectangle, opaque edge to edge - algorithmically
  rejectable). The baths are OUT of the candidate set.
- The other multi-direction files hold a few distinct views duplicated
  across slots, not six azimuths: car4b's six slots are two near-identical
  trios (124 px x3 then 134 px x3), car2b's are 142 px x3 then 99 px x3,
  truck2/truck3 alternate two views and hold placeholders in d4/d5.
- truck2/truck3's direction 2 is BYTE-EQUAL to the mirrored game view
  (IoU 1.00, equal pixel counts): the artists themselves used the flipped
  frame as the opposite view. That is the game's own convention for these
  vehicles.
- Cross-frame registration wanders up to 28 px between slots (the
  coordinate-descent search found it), so no metric hull can be consistent
  across views. This also explains why 122's card turning read poorly on
  cars: the six slots snap between two quarters.

The slice therefore builds the model from what the art truly holds, still
with no hand work and no name lists:

1. Qualify (per art, automatic): all stored directions decode; a direction
   whose frame is opaque edge to edge is a placeholder card and drops out;
   at least two real directions must remain (props.real_directions).
2. Fit from the game view as today: qualifying arts that refuse the plain
   fit take the forced box (the overrides.json path, cover at least 0.7)
   raised to the art's own height bands (props.carved: the length cuts into
   strips, each as high as the sprite rises over the strip's far edge, the
   bands are the body, the cabin). The solids stand fly-only (seen=1, the
   119 machinery, wall clips included), so the game's views keep their
   cards pixel for pixel.
3. Textures: nothing new needed - the box faces already sample what they
   land on and the away faces already mirror the near sides' pixels
   (solids.solid_faces), which is the same treatment the drum sides get.
   The art's own opposite view (the mirrored-equal direction, truck2 and
   truck3's d2) is a later per-face-source refinement.

Side finding for 122: truck2/truck3 pass the engine's turning gate while
holding placeholder directions - a turned truck card can show the green
UNUSED ART card. The turning gate needs the same placeholder rejection.

On this branch's stack the qualifying Den arts stood: CAR4b as two boxes
(body 0.77 tall, a second band 0.55), CAR3b as three bands, the car3 wrecks
as two- and three-band rows. The regression, the caches and the pairs below
ran on this branch (121's texture fill comes later in the stack and is not
part of these pairs).

### 2026-10-31 glm: the conversion is one algorithm, not a model shop

The owner asked the models come out as algorithmically as possible. So the
slice is a rule that runs itself over every art that qualifies, no
hand-built objects: the qualification reads the art's own frames (real
directions against placeholder cards), the geometry comes from the game
view's forced box and the art's own height profile, the faces texture by
the standing projection rules. No name lists anywhere.

### 2026-10-31 glm: the owner's addition: the multi-side big things become real 3D models

The owner, on the 122 turning pairs: «по машинам - их надо в 3д модели
конвертировать». The multi-side big things (the cars, the trucks) are not
for card turning - they convert to 3D models: fitted solids whose faces
texture from the art's own views instead of the single game-view
projection. This is this task's umbrella work; the candidate list comes
from the art itself (the qualification rule), not name lists.

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
