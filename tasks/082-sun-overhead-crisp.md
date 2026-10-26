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

### 2026-10-26 glm
Done: round 2 (the owner's eye pass) and the two [3d] knobs he asked for.

Round 2 (commit b8232ae0):
- Shadow density: the ground/wall darken factor 0.425 -> 0.35 by the
  owner's eye.
- The sun swings 30 degrees (was 15): at dawn and dusk it stands 60
  degrees above the horizon; the sun test rewritten (-30/0/+30 at
  6/12/18, -25/+25 at 7/17).
- Ctrl+T was forward-only, dropped the day count across midnight and had
  no readout (at night a step was invisible). Now Ctrl+T steps forward,
  Alt+T backward, the day is kept, the minute lands, and the new hour
  shows in the dev panel's status line.
- The base junction at 60 degrees: the pass-1 coplanar caster copy was
  capped at 0.4, leaving a residual gap for casters 0.4-0.6 above the
  floor. Near-floor casters (lowest <= 0.6) now drop fully onto the
  floor; high casters keep no copy (a roof dropped onto the floor would
  shadow the whole floor under it). The stripe did not reproduce at the
  long building even before the fix; the fix removes the class.
- Seams inside the shadow identified by the debug views: the dashed
  strokes under fences are the fence ART (its transparent holes cut out
  of the depth map, 063's design); the lit patch in the big morning
  shadow is the barrel card standing at its own height. No caster seams.
- Measured: 6 factor+face pairs, worst column step 0.0000, flatness
  1.0000, edge 2-4 game pixels. Sheets in
  runs/082-sun-overhead-crisp/round2/.

The knobs (the owner's request, modeled on shadow_pixel_size):
- [3d] shadow_density, default 1.0, range 0..2: scales the shadow
  darkening (0.35/0.3 multiplied by it); 0 turns the cast shadows off.
  Uniform uShadowDensity, set in shadow.cc next to uShadowStrength.
- [3d] sun_swing, default 30, range 0..60: the sun's degrees off
  vertical at dawn and dusk; sunAt/SunTracker::update take it as a
  parameter, updateSunFromGame passes the setting.
- Both have menu lines in the Lighting group (options_panel), cfg keys,
  defaults written on first run. Checked live: density 0 clears the
  ground shadows (wall shading stays, it is the faces' own look), 2.0
  darkens monotonically (shadow-zone luma 52.2 / 42.5 / 34.7 at 0/1/2);
  swing 0 presses every shadow under its caster, 60 stretches building,
  hydrant and fence shadows (masks in build/082-shots/knobs/). ctest
  14/14 (the sun test gained swing 60/0/clamp cases).

### 2026-10-26 glm (round 4)
Done: the owner's two finds from his eye pass (the lit pixel strip along the
shadow's base at the long building; the building corner the shadow does not
take). Commit 40b66fd2.

- Found and fixed a real defect: FOUNDATION-STONE casters sit BELOW the floor
  (walls_lowest histogram: lowest spans -0.44..+2.1, with 4.9k casters under
  0). They had no floor copy in any round (the old cap and both cutoffs
  required lowest > 0), so their art's transparent gaps kept lit holes at the
  contact zone; 55k depth-map pixels changed once their copies (lifted to the
  floor by a negative drop) started drawing. Fence lattice stays lattice.
- The copies now draw solid (vShadowSolid vertex attribute, 6th float in the
  caster buffer; the depth pass skips the alpha discard for them), the
  cutoff rose to 1.0 (kladwtwn slabs put wall quads up to ~1.0), and the
  copy break fires only on an exactly zero gap.
- The lit strip itself is NOT copies and NOT alpha (unchanged with solid
  copies; factor view shows full light 255 with a 2 px AA edge). Leading
  diagnosis: at buildings on a raised slab the slab's near edge owns the
  shadow depth map at the foot (its depth is nearer to the light than the
  wall's copy), so the wall's shadow cannot start at the wall; width
  follows 1/tan(height) (10 px at 68 deg). Fix is the next round's work
  (candidates: ground receivers near walls read a second-closest blocker,
  or path/slab cards stop casting at high sun).
- GOTCHA for devshots: an `if (false) discard` diagnostic in the fragment
  shader makes the compiler drop uCutoff, the static floor shader then
  reports "uniforms are missing" and NO snapshot is taken (silent 90 s
  timeouts). Diagnostics must keep every uniform referenced.
- ctest 14/14. Before|after sheet: runs/082-sun-overhead-crisp/round3/.

## Round 5 continuation (2026-10-26, the strip and the corner hunt)

- New diagnostics: -ShadowDebug 4 now reads the center tap's diff in texel
  units (0.05 light per 2*uShadowBias, signed); -ShadowDebug 5 buckets it
  (0, then brighter every texel-equivalent a blocker stands in front).
  The old gain (diff * 8) was invisible: diff lives in normalized depth,
  one map texel is about 1/250 of it.
- Measurement at the building spot, h7: the lit band's texels read EMPTY
  (10+ texels behind, that is the cleared far depth). A 2.0-world solidify
  probe of the walls' bottom band changed nothing there - the band's rays
  do not cross the wall plane at all.
- The occluder over that ground is the walkway canopy, a flat elevated
  wall-batch card: its art's soft border does not write depth, so its
  shadow detaches from its edge (the lit strip) and does not wrap its
  corner (the owner's open corner).
- Fixes (walls.cc): a flat elevated caster (top - lowest < 0.01, above the
  floor) writes the map solid; a vertical caster solidifies its bottom
  0.35 via a per-vertex 0.5 + depth-below-band-top attribute (the contour
  lands exactly at the band top for any triangle size). The scene's own
  draws leave the attribute at 0 and discard as before. The morning's
  six-texel floor-copy lift is reverted: the coplanar theory did not hold.
- tools: crop_box.py, image_diff.py, patch_pixel.py (the eye-review kit).
- Verification: ctest 14/14; shadow_stripes PASS 0.0000 / flatness 1.0000
  at 7, 12, 17; ground edge 3-4 px; the corrals' fence lattice intact by
  eye; the wall base hugs its shadow at 7 and 17 by eye.
- OPEN, needs the owner's eye: the wide lit band along the walkway at h7
  measures as genuinely sunlit ground (factor 0, the map empty along the
  whole ray to the sun) - our computed sun diverges there from the
  painted-original look. Shots: build/082-shots/r5-flat/.

## Round 6 (2026-10-26, the owner's spec, three general mechanisms)

The owner's eye pass on the round-5 build found four defects: gaps at wall
joints; a mesh of shadow appeared under the КУПЕЦЪ sign; contour gaps
between characters in shadow and their shadow; tile-square gaps on the
floor. His spec, recorded verbatim in spirit:

- No more special-case rules per object kind ("flat elevated card writes
  solid", "bottom 0.35 of a wall solid"). Everything closes through three
  general mechanisms; the rules they replace are removed afterwards.
- Mechanism 1, gap filling in the depth map: after the depth pass, one
  fullscreen pass dilates the map into a separate texture the receivers
  read. An empty texel is filled when, within 2 texels along one of the
  four axes (horizontal, vertical, two diagonals), occupied texels stand
  on BOTH sides; it takes the farther of the two (closer to the ground).
  The outer contour never grows (both sides required); holes wider than 2
  texels (fence lattices, true openings) stay. Measure the width on the
  corral lattice: it must stay a lattice, else use 1. Check first with
  -ShadowDebug 5 at a joint and on the floor: an empty map means this
  mechanism owns it; a solid map with the hole still there means the
  receiver, see mechanism 2's note.
- Mechanism 2, an object must not shadow itself (the mesh on the sign and
  on the wagon roof): the depth map gains a second channel with the
  caster's object number; the piece drawn on screen passes its own number;
  map texels holding it compare as empty. Check with -ShadowDebug 5 on the
  sign's mesh: differences up to 2 texels mean self-shadowing (mechanism 2
  closes it); more means another object casts (the sign's letters onto the
  wall behind), which mechanism 1 closes (the holes in the letters fill).
- After 1 and 2, disable the flat-card-solid and bottom-0.35 rules and
  re-measure the canopy and the corner from round 5. If the canopy gap
  stays gone, the rules are deleted; if it returns, they stay and the log
  records what exactly they are needed for. The foundations' floor copies
  (round 4) stay: they close the ground gap, different geometry.
- Mechanism 3, a sprite in shadow darkens whole at once (people, mobs,
  items, everything with uLightMode == 3): one shadow factor per sprite,
  taken at its true feet point (the bottom center of its card in world,
  following the walk step, not the hex center) plus a small lift; the
  25-tap disk samples in the ground plane, the sprite's own number
  excluded (mechanism 2), else it shadows itself at the feet. This
  deliberately reverses the 063 per-pixel decision (its hex steps came
  from the hex center; with the feet point the transition is smooth, the
  whole sprite darkens over about one texel of travel). The lit contour
  around a character in shadow should go with it; if it remains, it is
  another pass - find it with the debug view and describe it in the log,
  do not patch it. A large sprite that reads wrong all-dark: log it, not
  fixed this round.
- Acceptance: the four places from the owner's screenshots shot in the
  same views, a was|now sheet in runs/082-sun-overhead-crisp/round6/;
  a character walking into shadow as an 8-frame strip, the whole sprite
  changing together, no steps; the old measurements hold (shadow_stripes
  0.0000, face flatness 1.0000 at 7, 12, 17, edge 2-4 px, corral lattice
  intact); ctest passes; the original-view regression matches pixel for
  pixel.
- Limit: at most two attempts per mechanism; if gaps remain after two,
  log the -ShadowDebug 4/5 measurements at those places and stop - no
  third variant, no new special rules.
