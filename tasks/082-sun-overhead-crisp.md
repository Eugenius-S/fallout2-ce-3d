---
id: 082
title: The sun swings 15 degrees either side of overhead; shadows crisp, no stripes on walls
milestone: M1
status: doing
owner: glm
suggested: glm
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The sun stands straight overhead at noon and tilts 15 degrees off vertical
at dawn and dusk, in one plane; shadows have a crisp edge and an even
density; no wall, roof or card shows stripes from the shadow map at any
hour.

## Context

- The owner, 2026-10-25, with a shot of the long building at noon (`runs/inbox/2026-10-25-shadow-stripes.jpg`): every
  wall face is covered in vertical stripes. Their wish, word for word in
  substance: crisp shadows; the sun at its zenith perpendicular to the
  ground; in the morning -15 degrees, towards night +15 degrees. Their
  sketch (`runs/inbox/2026-10-25-sun-cone.png`): the sun only inside the
  cone 15 degrees either side of vertical, never lower.
- Builds on 062 (branch `task/062-shadows-short-steady-smooth`, PR #91):
  finish this there or on top of it, one PR.
- The sun's path today (`ce/src/scene/sun.cc` on that branch): the height
  goes 75 -> 90 -> 75 as a sine and the azimuth turns 180 degrees through
  the day, so at 9:00 the sun is only about 4 degrees off vertical and
  leans sideways. Wanted instead, linear in one fixed vertical plane:
  tilt = -15 + 30 * (hour - 6) / 12 degrees between 6:00 and 18:00, 0 at
  noon; direction = (sin(tilt) * axis, cos(tilt) up), `axis` a fixed
  horizontal unit vector, one named constant. Start with the game camera's
  screen horizontal, so shadows fall to one side of things on screen in
  the morning and to the other in the evening, never towards or away from
  the viewer. Night as today: no sun.
- Why the walls stripe (`scene_shader.cc`, `sunShadow`): the 25 taps
  spread in the ground plane (xz) around the fragment. On a vertical face
  half of them land inside the wall's own body, under its own top, and
  count as shadowed; with the map sampled NEAREST that half changes from
  one map texel column to the next, which is the stripes. The same
  happens on roof edges and cards. Fix it at the root:
  - a face turned away from the sun (N.L <= 0, N from
    `cross(dFdx(vWorld), dFdy(vWorld))` as the point lights already do)
    is in its own shadow: no map lookup, the shaded value;
  - otherwise move the lookup point off the surface along N by at least
    1.5 shadow map texels before comparing (normal offset), and spread the
    taps in the face's own plane, not in xz;
  - keep the bias as small as the offset allows.
- Crisp: with the sun at least 75 degrees high a 2-unit wall casts at most
  0.54 units of shadow, so 062's melt over hexes is no longer needed; one
  even density inside, an edge about one map texel soft. Check the shadow
  map's texel size near the camera and raise its resolution or fit its
  box tighter if the edge reads blocky.
- The game's own view of walls and floor without the sun must stay as it
  is; the sun is a [3d] setting with its menu line (it is).
- PR #91 carries about 90 PNGs in `runs/`: since 079 CI refuses files over
  1 MB there. Keep a few JPEG sheets, the rest in `game/shots/`.

## Acceptance

- `tools/dev-snapshot.ps1 -Shadows -Hour 7|12|17` on kladwtwn at the long
  building (`-ViewTile 24292`) and at the corrals, the game's view and one
  fly view each, in one before | after JPEG sheet in
  `runs/082-sun-overhead-crisp/`.
- No stripes: in the raw factor debug view (uShadowDebug 1) at 12:00 the
  wall fronts of the long building are even: the per-column mean of the
  factor along a wall changes by under 0.05 between neighbouring columns
  (a small script over the snapshot, its output in the task's Log).
- A sun unit test: tilt -15, 0 and +15 degrees at 6:00, 12:00 and 18:00,
  linear between, all in one plane.

## Out of scope

Cloud shadows (069), the look of light at night.

## Log

### 2026-10-25 glm
Done: the sun swings 15 degrees either side of overhead in one plane, the
shadows are crisp, the walls stripe no more. On top of 062 round 7, one PR.

- Sun (sun.cc): the tilt is linear, `-15 + 30 * (hour - 6) / 12`, 0 at noon;
  the direction is `sin(tilt) * kSunAxis + cos(tilt) up`. kSunAxis is the
  game camera's screen horizontal, a named constant: the ground step one
  screen pixel right moves a point across the screen and never up or down
  it, from the donor projection (kladwtwn's projection.bin: one ground u
  step is (-48, 12) game pixels, one v step (32, 24)), so the unit vector
  is (-2, 1) / sqrt(5). Morning shadows fall screen-right of things,
  evening screen-left, never towards or away from the viewer. Night as
  before. The unit test checks the new model: tilt -15 / 0 / +15 at
  6:00 / 12:00 / 18:00, linear between (+-12.5 at 7:00 and 17:00), one
  plane (the direction's dot with the plane's normal under 1e-5 at all
  hours), a unit direction, dawn and dusk leaning to opposite sides.
- Stripes (scene_shader.cc), the root fix as specified: a face with N.L
  <= 0 (N from cross(dFdx(vWorld), dFdy(vWorld)), camera-oriented, as
  pointLights reads it) takes the shaded value with no map lookup;
  otherwise the lookup point is lifted along N by 1.5 map texels and the
  25 taps spread in the face's own plane (a basis built from the normal),
  so no tap lands inside the wall's own body. The compare bias shrank to
  0.5 map texel of depth: the offset clears the surface, the bias covers
  the blockers' slope error within one texel. An epsilon of 0.001 (about
  0.06 degrees) in the gate turned out load-bearing: at noon N.L stands
  exactly at 0 and the cross product's rounding split the quads by sign
  into a flat-shaded half and a lit, map-reading half; the metric caught
  it (the gate held 140 of 163k face pixels before it, all of them after).
- Crisp: the melt is gone (shadowFadeTerm, uShadowGround and uShadowRange
  with it; -ShadowDebug:2 now shows the face test, the dev-snapshot
  comment updated). The shadow map is 8192: one texel is 0.015 GL units,
  under one game pixel at the camera (hpx about 51), the NEAREST stair is
  invisible. The AA ring is at least one map texel (was two), and the
  [3d] shadow_pixel_size default came down 5 -> 1, the knob keeps its
  meaning. Measured edge: the sharpest 10-90% transition in the picture
  is 2-4 game pixels wide, 3-5 map texels.
- No stripes, measured (tools/shadow_stripes.py: the factor from
  -ShadowDebug:1, the faces from -ShadowDebug:2 of the same view, the
  picture's border and interface eroded away, the void past the map edge
  excluded): the worst per-column step of the factor over the
  sun-turned-away faces is 0.0000 at the long building at 7:00, 12:00 and
  17:00 (game and fly views) and at the corrals at 7:00 and 12:00; the
  face flatness (the share of face pixels at the flat shaded value) is
  1.0000 everywhere, 51k-266k face pixels a view, 0 pixels between. The
  acceptance case (12:00, building, game view): worst column step 0.0000
  over 163,144 face pixels.
- Before | after: runs/082-sun-overhead-crisp/, 12 JPEG sheets (building
  and corrals, 7:00 / 12:00 / 17:00, the game's view and one fly view
  each; before is 062 round 7, after is this branch, both with the
  [3d] shadow_pixel_size the branch ships). The full-res PNGs stay in
  game/shots/dev/2026-10-25/.
