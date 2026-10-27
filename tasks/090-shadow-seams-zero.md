---
id: 090
title: Sun shadows with zero seams - no lit holes, no specks, no hatching on faces
milestone: M1
status: doing
owner: claude
suggested: claude
priority: high
claude-only: yes
user-check: yes
depends-on: [082]
reviewed: no
---

## Goal

The sun shadows that 082 landed show no seams anywhere: no lit pixels inside
a shadow, no shadowed specks in the light, no hatching or grid on a face, no
lit corner strip, no light lines along floor tile joints. Measured, not by
eye: a seam metric reads 0 at every checked view.

## Context

- 082 landed as round 6 (the caster-id channel, the gap-fill pass, sprites
  darkening whole at their feet) on main's repaired geometry (075, 076, 081,
  083). The owner rejected round 6's look and moved the seams here, to
  Claude, with the stance: fix the algorithm, no hand-made cases per object
  type.
- The owner's round 6 finds, in their game
  (`runs/inbox/2026-10-26-shadow-seams-01..06.png`):
  - 01: the BUCKETS (КУПЕЦЪ) sign wall: a grid under the sign, lit pixels
    at the wall base (red mark).
  - 02: a building corner ignores the shadow (a lit strip down the corner);
    lit pixels on the ground (red mark).
  - 03: the bathhouse wall, facing the sun: diagonal saw-tooth hatching over
    the whole face.
  - 04: lit dots stair-stepping along a shadow edge.
  - 05: the corral and the brahmin: lit specks on the ground near the
    brahmin (red mark); the fence shadows break up.
  - 06: floor tile joints read as light lines inside the shadow; lit star
    dots (red mark).
- Suspects read from the code at hand-over (`scene_shader.cc` sunShadow /
  shadowCoverage, `shadow.cc`), to confirm with the debug views 4, 5, 6
  before changing anything:
  - `sunShadow` returns fully lit when the center tap alone reads lit: any
    one empty or behind texel under a pixel inside a shadow makes a lit
    pixel, whatever the other 24 taps say (holes, specks, stars).
  - The filled map is RG16: the depth drops from 24 to 16 bits (one step is
    about 0.007 GL units, while the compare bias is about one step), so
    contact zones and coplanar casters compare on quantization noise.
  - Sun-facing faces at a grazing sun read texels written by coplanar
    neighbours (other wall objects, overlapping art): the id channel only
    drops the face's own object, the rest is classic acne (the hatching).
    A receiver-plane depth compare (the receiver's depth extrapolated to the
    texel center along its own plane) makes any coplanar caster compare
    equal, whoever wrote it.
  - The face gate is a knife edge (N.L above 0.001 reads the map, below
    takes the flat shaded value): a corner strip whose normal tilts a hair
    towards the sun turns fully lit next to a shaded face.
- Keep what 082 holds: shadow_stripes 0.0000, face flatness 1.0000, edge 2
  to 4 px, the corral fence lattice intact, sprites darkening whole at their
  feet, the original-view regression at 0 px.

## Acceptance

- A seam metric tool (extend `tools/shadow_stripes.py` or a sibling) over
  the raw factor views (`-ShadowDebug 1`, plus 2 for the faces): holes (lit
  pixels inside the shadow mask eroded by the AA edge width), specks
  (isolated shadowed pixels in the light) and face hatching (repeated stripe
  patterns across one face). It reads nonzero on the owner's six spots
  before the fix.
- After: the metric reads 0 at the owner's six spots and at 7, 12 and 17 h,
  the game view and one fly view.
- A before | after JPEG sheet of the six spots in `runs/090-shadow-seams-zero/`.
- ctest passes; the classic-view regression shows 0 px.

## Out of scope

Cloud shadows (023), the night look, the sun's path (082).

## Log

### 2026-10-26 claude: the metric and the baseline on main (c05a0f34)

- `tools/shadow_seams.py` reads a `-ShadowDebug 1` snapshot (the debug view
  now paints non-receivers magenta, the metric leaves them out) and counts,
  in picture pixels: holes (small lit islands and lit lines up to 2 px wide
  inside the shadow), specks (small shadowed islands in the light), dips
  (shadow pixels past the 4 px AA edge that miss the full factor) and grains
  (lit pixels past the edge that read some shadow). `--overlay` marks them:
  red holes, yellow dips, blue specks, cyan grains.
- On the owner's spots the overlays mark what the owner saw: window dots and
  piece joints on shaded faces, the light line at wall bases, the fence
  rails breaking up, the hatched wall rectangle.
- Baseline, the owner's six tiles at 7, 12 and 17 h, game view, plus the fly
  view at 24292 (two identical runs gave identical counts):

| view | holes | specks | dips | grains | total |
|---|---|---|---|---|---|
| 25075 h7 | 6045 | 544 | 651 | 27 | 7267 |
| 25075 h12 | 3517 | 122 | 0 | 0 | 3639 |
| 25075 h17 | 4079 | 1177 | 333 | 30 | 5619 |
| 24292 h7 | 6426 | 630 | 9 | 317 | 7382 |
| 24292 h12 | 3409 | 58 | 0 | 0 | 3467 |
| 24292 h17 | 3967 | 613 | 0 | 16 | 4596 |
| 24292 h7 fly | 10051 | 1105 | 955 | 108 | 12219 |
| 24292 h12 fly | 9270 | 251 | 278 | 0 | 9799 |
| 24292 h17 fly | 8701 | 930 | 610 | 1745 | 11986 |
| 23481 h7 | 8653 | 806 | 557 | 354 | 10370 |
| 23481 h12 | 4200 | 93 | 0 | 0 | 4293 |
| 23481 h17 | 3883 | 1092 | 2 | 15 | 4992 |
| 19265 h7 | 5019 | 369 | 5483 | 8 | 10879 |
| 19265 h12 | 5394 | 660 | 6606 | 0 | 12660 |
| 19265 h17 | 5323 | 670 | 6278 | 7 | 12278 |
| 17104 h7 | 5346 | 89 | 0 | 74 | 5509 |
| 17104 h12 | 4987 | 172 | 0 | 0 | 5159 |
| 17104 h17 | 3482 | 639 | 0 | 244 | 4365 |
| 20690 h7 | 7252 | 646 | 9 | 402 | 8309 |
| 20690 h12 | 3819 | 109 | 2617 | 0 | 6545 |
| 20690 h17 | 3502 | 494 | 2396 | 121 | 6513 |

- Found in the code: `shadowMatrix` gave uv and depth in 0..1 while the
  caster pass uses it as the clip-space projection, so the casters filled
  only the map's upper right quarter (a 4096 map in effect) and
  `uShadowTexel` was 2 to 2.8 times smaller than the real texel.

### 2026-10-26 claude: round 1, total 157,846 -> 53,347 seam pixels

- Root cause of most holes: the shader took the face normal's screen
  derivatives after the alpha `discard`. In a 2x2 quad with a discarded
  neighbour (the floor tiles' transparent joints, a window's art holes, a
  wall's base) NVIDIA gives zero derivatives, the normal turned NaN, the
  face gate and every compare failed, and the pixel read lit inside the
  shadow. Debug view 2 now paints such pixels red: 69,601 at 24292 h7
  before, 0 after taking the derivatives at the top of `main()`. This is the
  owner's 01 (lit pixels at the wall base), 04 and 06 (joints, star dots).
- `shadowMatrix` gives clip space, so the casters fill the whole 8192 map;
  the texel is the real one; the gap fill reaches 4 texels (the same width
  on the ground as 082's 2).
- The filled depth is R32F (was 16 bits in RG16), the caster ids a separate
  R16 on unit 6, written by the fill pass as a second target.
- The receivers compare their own plane carried to the texel center (the
  receiver-plane compare): no normal offset, no center-tap early out, a
  quarter-texel bias. The hatching on sun-facing faces (03) is gone.
- Cards (doors, signs, people) take their feet's majority, in or out: a door
  in its wall's line read a fifth or a half dark against the wall (the
  dips at 19265 and 20690, 7551 and 3045 px at noon, now 16 and 0).
- The same derivative bug sits in the point lights' wall normal
  (`sceneLighting`); fixing it there changed 179 to 1155 px in the caves
  (klaratcv, klatoxcv), so it stays out of this task and goes to the inbox.
- ctest 16/16; regression 0 changed px on all 16 views.

| view | before | holes | specks | dips | grains | after |
|---|---|---|---|---|---|---|
| 25075 h7 | 7267 | 3354 | 552 | 17 | 44 | 3967 |
| 25075 h12 | 3639 | 3656 | 15 | 0 | 0 | 3671 |
| 25075 h17 | 5619 | 2911 | 909 | 12 | 21 | 3853 |
| 24292 h7 | 7382 | 1736 | 535 | 87 | 46 | 2404 |
| 24292 h12 | 3467 | 1823 | 4 | 173 | 0 | 2000 |
| 24292 h17 | 4596 | 1225 | 499 | 236 | 13 | 1973 |
| 24292 h7 fly | 12219 | 3505 | 1147 | 2 | 330 | 4984 |
| 24292 h12 fly | 9799 | 2828 | 210 | 106 | 0 | 3144 |
| 24292 h17 fly | 11986 | 2816 | 971 | 170 | 219 | 4176 |
| 23481 h7 | 10370 | 2754 | 594 | 14 | 87 | 3449 |
| 23481 h12 | 4293 | 2617 | 50 | 0 | 5 | 2672 |
| 23481 h17 | 4992 | 1402 | 845 | 5 | 23 | 2275 |
| 19265 h7 | 10879 | 1827 | 396 | 6 | 132 | 2361 |
| 19265 h12 | 12660 | 2023 | 692 | 16 | 7 | 2738 |
| 19265 h17 | 12278 | 2090 | 694 | 98 | 83 | 2965 |
| 17104 h7 | 5509 | 836 | 67 | 22 | 173 | 1098 |
| 17104 h12 | 5159 | 368 | 95 | 389 | 0 | 852 |
| 17104 h17 | 4365 | 773 | 521 | 244 | 246 | 1784 |
| 20690 h7 | 8309 | 748 | 372 | 20 | 170 | 1310 |
| 20690 h12 | 6545 | 588 | 86 | 0 | 1 | 675 |
| 20690 h17 | 6513 | 400 | 506 | 1 | 89 | 996 |

- What is left, read from the overlays:
  - wall caps: the 1 px top face of a wall box and of every fence rail
    faces up and reads lit while its wall reads the flat shaded value (all
    of 25075 h12, most of 24292 h12; the owner's 02 corner strip is the
    same kind on a side face);
  - fence rails and foliage: their art's alpha holes make the shadow on the
    ground ragged and dotted (05, the tree at 19265); a wider gap fill (6
    texels) changed nothing, so these are not map gaps;
  - a door card on a wall turned from the sun still reads lit at noon (one
    factor at its feet, the ground there is lit); not counted by the metric
    but visible.

### 2026-10-27 claude
Paused for the night with round 2 work in progress (uncommitted code from
after 504d2970, saved as a WIP commit, not yet measured or regression
checked). Open question to the owner: count the lit 1 px wall and rail caps
as seams and shade them with their wall, and close small alpha holes in
fence and foliage art when building the shadow map while keeping real gaps.
Next: get the answer, finish round 2, measure, then ctest and the regression.

### 2026-10-27 claude, round 2
Owner's answers: the lit caps count and take their wall's factor; small
alpha holes close in the map, real gaps stay; the noon door card is fixed
the same way only if its cause is shared, else logged; foliage looks fine.
- Signs and other cards hung flat on a wall (within 0.14 of an overlapping
  wall face, 24 on kladwtwn, the Buckner sign among them) cast nothing: the
  wall's shadow covers them.
- The face gate reads the painted plane: a wall's 1 px top, a rail's top
  and a corner strip take their front face's side of the sun. A cap is
  carried onto its painted plane, one disk radius down, and takes that
  face's factor there, in or out.
- The metric leaves out the faces turned from the sun (blue): the ground
  seen lit between two shaded fence rails is light, not a hole.
- Low live cards (straw, grass, flat rubble) read per pixel on the ground
  under their line of sight; one factor per card was a patchwork.
- Contact: a blocker within 16 texels of depth of the receiver casts
  nothing there, and a tap past such a foot does not count. The ground seen
  through the stepped transparent foot of a wall's sprite lies just behind
  its face and read the wall: a dotted dark line along every wall's foot
  in the evening (25075 h17 specks 1068 to 75). On the ground only it lit
  less at the eave but left more at the fly view's wall corners (24292
  2295 against 3055 px over its six views), so it covers every receiver.
- The depth pass closes art holes up to 3 texels (opaque on both sides
  within 2 along a row, a column or a diagonal). It changes little: a gap
  of 8 moved 24292 h7 fly by 14 px, so the ragged edges are not art holes.
- The noon door card is not the caps' cause: a standing card takes one
  factor at its feet, the ground there is lit at noon. Shading it like its
  wall means the face test for every standing card, people included, who
  would all go dark at noon. Logged, not done.
- ctest 16/16; regression 0 changed px on all 16 views.

| view | holes | specks | dips | grains | total |
|---|---|---|---|---|---|
| 25075 h7 | 400 | 123 | 1 | 105 | 629 |
| 25075 h12 | 0 | 0 | 0 | 0 | 0 |
| 25075 h17 | 246 | 75 | 0 | 10 | 331 |
| 24292 h7 | 91 | 156 | 0 | 1 | 248 |
| 24292 h12 | 0 | 0 | 0 | 0 | 0 |
| 24292 h17 | 152 | 44 | 1 | 2 | 199 |
| 24292 h7 fly | 572 | 328 | 0 | 2 | 902 |
| 24292 h12 fly | 24 | 11 | 0 | 0 | 35 |
| 24292 h17 fly | 657 | 244 | 0 | 9 | 910 |
| 23481 h7 | 105 | 164 | 5 | 9 | 283 |
| 23481 h12 | 0 | 1 | 0 | 5 | 6 |
| 23481 h17 | 180 | 73 | 0 | 6 | 259 |
| 19265 h7 | 914 | 141 | 0 | 2 | 1057 |
| 19265 h12 | 931 | 137 | 7 | 0 | 1075 |
| 19265 h17 | 1457 | 285 | 8 | 0 | 1750 |
| 17104 h7 | 110 | 9 | 5 | 0 | 124 |
| 17104 h12 | 2 | 0 | 0 | 0 | 2 |
| 17104 h17 | 349 | 63 | 1 | 1 | 414 |
| 20690 h7 | 132 | 103 | 0 | 2 | 237 |
| 20690 h12 | 0 | 0 | 0 | 1 | 1 |
| 20690 h17 | 105 | 103 | 0 | 0 | 208 |

- What is left, read from the overlays:
  - live cards against a shaded wall: a tree's card takes its feet's lit
    factor and its twigs read as holes in the wall's shadow (787 of 19265's
    931 noon px); at 25075 h7 most of what is left lies on live cards;
  - window grates: their bars' shadows on the pane behind read dotted;
  - fence rail shadows on the ground keep ragged lit dots at their edges.
- Not done: the owner accepts at 0 on every view.

### 2026-10-27 claude, round 3
- The noon door: a card standing in its wall's plane (a door, the ones
  with a baked plane) takes that wall's side of the sun, the flat shaded
  value on a wall turned from it. The tree's shaded twigs at 19265 read
  against the lit door: 19265 h7 1057 to 157, h12 1075 to 175. People and
  free-standing things are unchanged.
- Review (PR 126): a low card under the fly camera projected its pixels to
  the ground along the game's direction; it now follows the free camera's
  own ray (24292 fly 902/35/910 to 886/34/909).
- The tree at 19265 h17 (1436 of 1750 px) stands in shadow in front of a
  sunlit wall: read per pixel at its own place it reads the same shadow
  (1457 holes either way), so its twigs over the lit wall are its own
  outline, not a seam. The per-pixel rule changed nothing and is not kept.
- ctest 16/16; regression 0 changed px on all 16 views.

### 2026-10-27 claude, round 3 (metric)
Decision (parent session for the owner, option a with two limits): the
metric leaves out only the pixels a standing live card covers (a tree, a
person: a live card darkened whole at its feet; doors in their wall stay
counted). Why: a tree standing in shadow before a sunlit wall shows its own
outline there, not a gap in a shadow, and the owner finds foliage fine.
- The factor view paints such a card green; shadow_seams.py leaves its
  pixels out and judges the patches round it with the card joined: a lit
  patch closed by shadow and the card is a hole, a lit line between a card
  and its shadow is a hole, the wall between two twigs is not, nor a line
  the card only crosses on its way to a left-out face.
- `py -3 tools/shadow_seams.py --self-test` draws the cases (a figure in
  shadow, a lit gap round its feet, a lit hole and a shaded speck beside
  it, a shaded tree over a lit wall): all pass.
- Proof on real frames, 19265 h17: 200 lit pixels painted under the cards
  standing in shadow raise the count 380 to 564; the contact rule off
  raises it 380 to 759, the part within 6 px of a card 0 to 8.
- The egg and translucent object passes upload each object's anchor,
  ground and wall parameters too (PR 126 review).
- Tried and dropped: stretching the disk's axes to a full radius across
  the map on grazed faces (the eave's teeth stayed, 300 holes, dips 8 to
  182). The teeth under an eave are in the raw coverage itself, a comb
  with a 3 px period: the map's stair along the roof's edge.
- ctest 18/18; regression 0 changed px on all 16 views.

| view | holes | specks | dips | grains | total |
|---|---|---|---|---|---|
| 25075 h7 | 166 | 72 | 0 | 96 | 334 |
| 25075 h12 | 0 | 0 | 0 | 0 | 0 |
| 25075 h17 | 207 | 73 | 0 | 9 | 289 |
| 24292 h7 | 90 | 141 | 0 | 1 | 232 |
| 24292 h12 | 0 | 0 | 0 | 0 | 0 |
| 24292 h17 | 128 | 40 | 1 | 2 | 171 |
| 24292 h7 fly | 284 | 280 | 0 | 1 | 565 |
| 24292 h12 fly | 8 | 0 | 0 | 0 | 8 |
| 24292 h17 fly | 405 | 203 | 0 | 9 | 617 |
| 23481 h7 | 99 | 138 | 5 | 9 | 251 |
| 23481 h12 | 0 | 0 | 0 | 0 | 0 |
| 23481 h17 | 154 | 69 | 0 | 6 | 229 |
| 19265 h7 | 116 | 4 | 0 | 2 | 122 |
| 19265 h12 | 29 | 0 | 7 | 0 | 36 |
| 19265 h17 | 297 | 75 | 8 | 0 | 380 |
| 17104 h7 | 118 | 9 | 5 | 0 | 132 |
| 17104 h12 | 0 | 0 | 0 | 0 | 0 |
| 17104 h17 | 345 | 50 | 1 | 1 | 397 |
| 20690 h7 | 83 | 103 | 0 | 2 | 188 |
| 20690 h12 | 0 | 0 | 0 | 1 | 1 |
| 20690 h17 | 63 | 87 | 0 | 0 | 150 |

- What is left: the eave's teeth on the wall below a roof's edge (morning
  and evening, the largest share at 17104, 19265 and the fly view); lit
  slivers between fence rail shadows on the ground (25075 h7); low plants
  and straw at a shadow's edge. Not done: the owner accepts at 0.
