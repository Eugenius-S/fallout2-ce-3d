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
