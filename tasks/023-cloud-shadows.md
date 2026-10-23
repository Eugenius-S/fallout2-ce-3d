---
id: 023
title: Cloud shadows
milestone: M1
status: done
owner: glm
suggested: glm (045's shadows landed in PR #73, the branch stacks on it)
claude-only: no
user-check: yes
depends-on: [045 (PR #73, stacked)]
reviewed: no
---

## Goal

Soft cloud shadows drift across the ground in daylight.

## Context

There are no visible clouds (no first-person view); only their shadows. They
dim the sun's light only, not lamps; the sun term comes from task 021.

## Acceptance

- A tiling cloud pattern moves slowly with a wind direction across floor,
  walls and sprites (sprites by their anchor, like building shadows).
- No cloud shadows at night or indoors; their strength follows the sun.
- `[3d] cloud_shadows` (default on) with a menu label.
- A short sequence of snapshots in the PR shows the motion.

## Out of scope

Weather.

## Log

## Log

### 2026-10-23 glm
Done: soft cloud shadows drift over the 3D view. The fragment shader grows
a seamless two-octave value noise over a 48-unit world tile (cloudDim);
the floor and the walls dim at the fragment, sprites at their hex's ground
point (uLightAnchor), by up to about a third of the sun's strength with a
soft edge. The wind drifts with game time (about a world unit a game
minute) and wraps the tile; no clouds at night, indoors, or with the sun
off (the sun's strength gates everything, as in 045). The cloud pass lives
in shadow.cc's bindStaticShadows: it needs no depth map, only uniforms.
[3d] cloud_shadows (default on) with a menu label in the sun's group; the
pinned test config keeps it off, so captures stay byte-stable; -Shadows in
dev-snapshot.ps1 turns the sun's whole effect on (shadows and clouds).
Check: the drift sequence runs/045-building-shadows/clouds-t1200/t1212/
t1224.png (12:00, 12:12, 12:24 at the corrals; the yard's and the roof's
cloud patches clearly move) and clouds-sequence.png stacked. Regression
-Spot entrance: 501 changed pixels, exactly the 045 baseline number, so
with the clouds off the shader is pixel-identical. Build, ctest 12/12.
Next: the owner judges the look and the drift speed (user-check: yes; the
speed is a constant pair in shadow.cc's cloud offset).
