---
id: 082
title: The sun swings 15 degrees either side of overhead; shadows crisp, no stripes on walls
milestone: M1
status: todo
owner: none
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
