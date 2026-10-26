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
