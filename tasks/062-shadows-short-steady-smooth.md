---
id: 062
title: Sun shadows end a few meters past their caster, hold still and blend smoothly
milestone: M1
status: doing
owner: glm
suggested: glm
claude-only: no
user-check: yes
depends-on: [063]
reviewed: no
---

## Goal

The owner's next round on the live 063 build (2026-10-25). A shadow is dense
at its caster and gone a few meters past it at every hour, so sunrise and
sunset no longer lay long shadows across the map. It never shows as a crisp
shadow one moment and a cut-off piece the next, it fades without visible
steps or layers, and the night shows no sun shadow at all.

## Context

- The owner's screenshots (2026-10-25):
  `runs/inbox/2026-10-25-shadow-partial-day.webp` (the long building by the
  pen: only part of its shadow is drawn, and the fade reads in layers),
  `runs/inbox/2026-10-25-shadow-pen.webp` (the cows in the pen),
  `runs/inbox/2026-10-25-shadow-night.webp` (dusk or night: dark bands and
  hard-edged light patches on the sidewalk read wrong).
- The look they want, their words: the shadow is dense at the object and melts
  to zero within a few meters. Take "a few meters" as full density for the
  first 2 hexes past the caster's foot and zero by 5 hexes, on the ground,
  at every sun height. The constants stay one line each.
- The snapshots do render the shadows: `tools/dev-snapshot.ps1 -Shadows
  -Hour 17`. A background run takes [3d] shadows from `--dev-shadows` and
  ignores the config, which is why 063's config A/B saw no difference. Judge
  every step from your own shots first; the owner's eye is the last check,
  not the first.
- Where to look first (hypotheses, check each with a shot before fixing):
  - Layers: each of the nine taps in `sunShadow()` (scene_shader.cc) adds
    either nothing or a nearest-texel fade, so the edge and the melt come
    out in up to nine flat steps. A `sampler2DShadow` with `GL_LINEAR` and a
    compare mode gives bilinear PCF for free (GL 4.1 has it). Measure the
    melt from one blocker depth averaged over the taps, not from each tap's
    own nearest texel.
  - A cut-off piece: the roofs cast through `roofShadowCasters()`. A roof
    that is cut away (the see-through roofs, the egg) or not loaded yet may
    drop out of the caster pass, so the building's shadow loses its roof
    part. Also check the shadow box (`shadowMatrix`, 50 + pad units around
    the origin): does it cover the whole visible map at a low sun?
  - The box's texel size changes with the sun's height (pad grows as the
    sun sinks), so the edge's softness and the AA ring change with the hour.
  - Night: `Sun::up` should be false from dusk to dawn. Find out what draws
    the dark bands in the night shot: a low sun still above the horizon
    fade, the lamps (020), or the game's own light map.
- The shadow code: `ce/src/scene_renderer/shadow.cc` (the box, the caster
  pass), `scene_shader.cc` (`shadowPast`, `sunShadow`, `shadowDarken`),
  `ce/src/scene/sun.cc` (the sun per hour, the horizon fade).

## Acceptance

- Shots under `runs/062-shadows-short-steady-smooth/` of the corrals and the
  long building at 6, 8, 12, 17 and 19, plus 22 and 3 at night, each against
  the same shot without `-Shadows`: past 5 hexes from a caster's foot no
  pixel is darker than in the shot without shadows; at 22 and 3 the two
  shots are identical.
- The same view three times in one run (the hero steps a hex between them):
  the shadow's outline moves only by that step, no pieces appear or vanish.
- A zoom-2 crop of a shadow's edge and its melt shows no flat steps.
- The regression with shadows off stays where it is.
- The owner's eye finishes it (user-check: yes).

## Out of scope

Cloud shadows (069), the lamps' own look beyond what the night shot shows.

## Log

### 2026-10-25 glm
Done: the first pass over all four lines.
- Length: the rim is measured on the ground now: full density to 2 hexes
  (1.0 GL unit) past the caster's foot, zero by 5 hexes (2.5 units), at
  every sun height: uShadowGround (the sun elevation's cosine) scales the
  ray distance down, so sunrise and sunset lay the same short rim as noon
  instead of a geometric shadow that outgrew the old ray window. That
  window is also the "whole shadow one hour, a cut piece the next": at low
  sun the old rim cut a long geometric shadow off mid-yard, at noon the
  whole shadow fit inside it.
- Layers: the depth map samples LINEAR, each tap compares through a
  smoothstep over the bias window instead of a hard nearest-texel cut, the
  melt comes from ONE blocker depth (the center tap) instead of nine
  per-tap fades, and the ring grew to two radii, 17 taps: a numeric edge
  profile showed the 9-tap ring quantizing the edge into about five flat
  steps a ring-radius apart (7 px runs at zoom 2). The isolated
  shadow-mask edge now reads a ~10 px transition; some 2-game-px
  quantization remains in the profile numbers, the owner's eye judges
  whether it still reads as layers.
- Night: no code needed: sunAt() gives strength 0 outside 06:00..18:00, so
  sun shadows cannot draw at 22 or 3 (the on/off pairs are byte-identical).
  The dark bands in the night shot are not sun shadows: the lamps line
  (020), out of scope here.
- Cut-off pieces: the box measured innocent (a numeric walk of
  shadowMatrix with the 17:00 and a dusk-4-degree sun keeps roofs and wall
  tops inside the box; roof casters draw from all squares unconditionally),
  and the hero-step triple (10-tile steps at 17:00) moves 7..20 k pixels
  against a 266 k shadow mask: the hero card and his egg, no shadow pieces
  appearing or vanishing.
Check: build Release, ctest 14/14. Runs under runs/062-shadows-short-steady-smooth/:
on/off pairs at hours 6, 8, 12, 17, 19, 22 and 3 (mask 341 k at 8:00,
409 k at noon, 267 k at 17:00, 0 at 6, 19, 22 and 3), the hero-step triple,
the fence zoom-2 on/off pair for the edge profile. The shadows-off
regression: the same 13 drifted spots at the same counts as before this
task, three at 0.
Next: the owner's eye (user-check: yes). Knobs one line each: rim end 2.5,
rim start 1.0, ring floor 2 texels, softness default 3. If layers remain,
the next step is compare-mode PCF (sampler2DShadow), which needs
sampler-object procedures in gl_procs.h.

### 2026-10-25 glm
Done: round 2, the owner's verdict on round 1 (his screenshots stayed in
the chat): the fade appeared "from the center, not from the object" at some
sun-to-wall angles, layers inside the shadows, stairs in places.
- The center fade was REAL and it was the bilinear depth of round 1: near a
  caster's silhouette the interpolation mixes the caster's depth with the
  empty far depth, so the contact line read a phantom far blocker, went
  light, and the shadow kept its density only in its middle. The map is
  back to NEAREST (the v2-to-v3 diff confirms the repair: the changed
  pixels sit 61% inside the shadow mask, at the contact lines).
- The disk grows to three radii, 25 taps (1.0, 0.65 at 22.5 degrees, 0.35
  at 11.25): the tap grid no longer quantizes the edge.
- What the edge profile metric still shows (about 3 levels in 10 px, 4 px
  runs) is the floor art's own texel grid, 2 x 2 screen pixels at scale 2:
  a 10 px edge spans about five art texels and cannot read smoother than
  that. So the default edge softness rises from 3 to 5 game pixels (the
  owner asked for more smoothing), the knob stays in [3d].
Check: build Release, ctest 14/14; the shadows-off regression keeps the
same 13 drifted counts, three at 0; the final zoom-2 pair under
runs/062-shadows-short-steady-smooth/ (fence-zoom2-final/-off, mask 610 k).
Next: the owner's eye again.

### 2026-10-25 glm
Done: round 3, the owner's verdict on round 2 (straight shadows without a
fade, stairs on the cut end, "you are fundamentally messing up somewhere").
He is right, and the debug view found it.
- New diagnostics, committed with the task: --dev-shadow-debug=N
  (dev-snapshot -ShadowDebug N): 1 the raw factor, 2 the fade term alone,
  3 the coverage term alone, drawn flat gray without art or the engine's
  light steps. This is how I can look at the algorithm without eyes for
  screenshots (this agent cannot open pictures; the numbers read the
  view instead).
- THE root cause of every "no fade / from the center / straight" round
  since 063: the wall caster quads sit about 0.37 GL units ABOVE the
  floor (the art's own placement), so the ground within
  0.37/tan(elevation) of a wall was never blocked: at 17:00 that is a
  1.35-unit lit gap between the object and its own shadow, and the shadow
  floated off its object as a thin fading band (the round-2 "fade from
  the center"). Noon hid it (0.2 units, inside the plateau). Measured
  with mode 2+3 at 17:00: the minimum ground distance of any deep-shadow
  pixel was 1.35 units, at noon 1.0 (the plateau edge); after the fix the
  plateau holds 12% of the deep shadow and the melt spans 1.05..2.5.
- Fix: every caster triangle is extruded into a prism down to the floor
  (the triangle, its floor footprint and three side curtains, the art's
  UVs kept so the transparent holes still cut); and the lit test lost the
  compare bias it inherited (past <= 0, not past <= bias), which alone
  kept a 0.23-unit contact ring lit.
Check: build Release, ctest 14/14; the shadows-off regression keeps the
13 drifted counts; the 17:00 on/off mask 381 k (the contact strip now
shadows), 22:00 mask 0 (night unchanged). Shots: debug-17-fix-mode{2,3},
fix-sunset-17{,-off}, fix-night-22{,-off} under this runs folder.
Next: the owner's eye.

### 2026-10-25 glm
Done: round 4, the owner's verdict on round 3 ("absolutely the same, stairs
on the shadows, the fade does not work on them at all").
- Read from HIS screenshots (the files sit in the chat cache; this agent
  reads them as data, pixel by pixel): the default 8:00 view is dominated
  by shaded WALL FACES, and the debug modes on the same view showed why
  they can never fade: a wall's own surface writes the depth map, so the
  ray distance past a blocker is not measurable on it; the whole shaded
  face rendered as one flat full-density mass (93% of the deep shadow at
  fade = 1) with the quad-row edges reading as stairs. The prism of round
  3 made it flatter still (the curtains put the blocker at the ground
  everywhere).
- The prism is reverted; the walls (light mode 2) no longer take the sun
  shadow at all: they keep the engine's painted shading, and the flat
  dark masses are gone from the 3D view. The rim fade stays where it can
  be measured and seen: the ground (mode 1) and the sprite cards (mode
  3). Wall shadows need a surface-distance design (fade along the face
  from the shade line) before they can come back; that is future work,
  not this task.
Check: build Release, ctest 14/14; the shadows-off regression keeps the
13 drifted counts; pairs under this runs folder: spawn8fix{,-off} (the
owner's default view: bright-base ground bands with a median 48-gray
drop), corrals17fix{,-off}.
Next: the owner's eye on the relaunched game: what is left of the sun
shadow is ground and card rims that fade; walls are painted-shaded only.

### 2026-10-25 glm
Round 5: the owner on round 4: "the wall shading on the shadow side is
gone", then "the shadow is there, the dimming is not". The walls go back
into the sun shadow (mode 2, 0.5 like the ground); the prism stays
reverted. Measured without the prism (spawn view, 8:00, modes 2+3): a
shaded wall face self-writes its own depth at its uv (the wall is the
first caster along its own sun ray), so its measured distance past a
blocker is near zero and the whole face takes the plateau: the dimming
is FLAT by construction, which is also how the game's own painted
shading reads. If a graded wall dimming is ever wanted it needs a
surface-distance design (fade down the face from the shade line), noted
as future work. His two verdicts say the flat dim is what he wants back.
Check: build Release, ctest 14/14; spawn8-noprism-mode{2,3} under this
runs folder (93.8% of the wall dim at full density, 6.2% melting at the
edges); game relaunched on the restored build (exe 844b9315).
Next: the owner's eye.

### 2026-10-25 glm
Done: round 6, the owner's plan (his words: cap the sun's swing at about
+/-15 degrees stretched over the day, smooth the straight shadow runs, no
stairs, opacity down 15 percent; the moon is future work, already in the
inbox).
- sun.cc: kNoonHeight 60 -> 15 (one constant; the swing runs over the
  day arc, so 7:00 reads 7.5 degrees, noon 15, 17:00 3.9 with the horizon
  fade dimming it to half strength). A low sun lays long geometric
  shadows everywhere, so the rim's melt always cuts them: no hard
  straight geometric edge is left in view. sun_test updated for the new
  arc (noon 15 degrees, warm at noon: the sun never climbs high).
- The shadow opacity drops 15 percent: 0.5 -> 0.425 for the ground and
  the walls, 0.35 -> 0.3 for the cards.
- walls.cc: every caster triangle also draws a copy shifted straight down
  (at most 0.4, never below the floor): with the sun now always low, the
  art's 0.37-unit lift would leave a lit strip between every wall and
  its shadow at every hour. The copy stays in the triangle's plane, so
  the measured distance from the caster does not change.
Check: build Release, ctest 14/14; the shadows-off regression keeps the
13 drifted counts. Pairs under this runs folder: spawn-low (8:00,
elev 7.5), corrals-low (17:00, elev 3.9), noon-low (12:00, elev 15):
edge samples across the masks read 2306 soft vs 27 hard at the spawn,
303 vs 1 at the corrals, 2206 vs 18 at noon; the median shadow drop
halved to 24 gray levels (the lighter 0.425).
Next: the owner's eye on the relaunched game.
