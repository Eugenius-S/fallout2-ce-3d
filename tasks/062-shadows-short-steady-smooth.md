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
