---
id: 045
title: Buildings and big things cast sun shadows
milestone: M1
status: doing
owner: glm
suggested: gpt (bailed 2026-10-23, the owner passed it to glm)
claude-only: no
user-check: yes
depends-on: [021]
reviewed: no
---

## Goal

Sharp sun shadows from buildings and big things fall on the floor and on
walls, and follow open doors, hidden roofs and changed objects.

## Context

- The sun comes from 021.
- Casters, by the owner (`docs/2026-10-20-shapes-and-rotation-design.md`,
  "Shadows"): walls with their backs and ends, roofs (hidden ones too, see
  013), doors, and big solids (at least two hexes across or taller than half a
  wall). Small things cast nothing and keep the shadows painted into their art.
- One directional shadow map (a depth texture, OpenGL 4.1) over the visible
  ground plus a margin. Receivers: the floor and walls.
- The original style: snap shadow lookups to a world grid of N game pixels
  (`[3d] shadow_pixel_size`), stable while the camera and the sun move.

## Acceptance

- `[3d] shadows` and `[3d] shadow_pixel_size` with menu labels; with shadows
  off the game view is exactly as before (the regression passes).
- Snapshots in `runs/045-building-shadows/`: noon, sunset and night at the
  regression spots, and an open door.
- The frame time with shadows on and off in the `Log`.
- A quick time of day, asked by the owner: a dev key in the panel steps the
  game hour (for example by 3 hours) and `--dev-hour=H` sets it on start, so
  morning, noon, sunset and night can be looked at in a minute; the sun is
  set at once, as on a time jump (021).

## Out of scope

Characters in shadow and the painted shadows (046), clouds (023).

## Log

## Log

### 2026-10-23 glm
Done: sun shadows from the static scene. ce/src/scene_renderer/shadow.cc:
one 2048 depth map over the whole map ground (the box grows with the sun's
lean, clamp 8..96), the casters drawn with the scene program itself in its
pick mode (uMvp = the shadow matrix, uPick = 1, colour mask off), then the
scene samples it: the floor and the walls (light modes 1 and 2) darken by
half the sun's strength inside the shadow, through the engine's own 128-step
darkening. The lookup snaps to a world grid of [3d] shadow_pixel_size game
pixels (default 4, hpx from the projection), so the edges stay put while the
camera and the sun move; the compare bias covers the snap window. Casters
(walls.cc, built at load, one buffer): wall batches (backs and ends in,
fly-only and game-only alike) always; other baked objects when they span two
hexes or stand taller than half a wall (the wall height comes from the walls
themselves), which brings the doors along; small props keep their painted
shadows. Roofs: all squares, hidden ones included (roofs.cc keeps a second,
visibility-independent buffer). Sprites do not cast and do not receive
(046's territory); roofs do not receive (they draw at the ambient level).
[3d] shadows (default on) and [3d] shadow_pixel_size with menu labels, in
the sun's group of the [3d] panel. With shadows off nothing of the pass
runs and the view is byte-stable; tools/test-config/fallout2.cfg pins
shadows=0, so captures stay as they were. Dev hour: a key in the panel
(T forward, Shift+T back, 3 hours a step, the sun snaps to the jump) and
--dev-hour=H on start; tools/dev-snapshot.ps1 grew -Shadows and -Hour.
Check: live captures in runs/045-building-shadows/ (noon, sunset at the
entrance and the corrals, night, the doors spot; the on/off pair at the
corrals differs in 647k pixels of 2M, the diff heatmap shows the building's
shadow across the yard, walls shaded by their own roof, fence posts casting).
Frame time: vsync-capped ~60 fps with shadows on and off alike; the pass's
CPU cost 0.05-0.2 ms a frame (devperf.log in the folder; the late 20+ ms
windows are the snapshot-capture frames polluting the averages). Live-opened
doors: the baked closed door keeps casting until a live caster path exists
(closed doors are shown casting; the open-door interactive shot is the
owner's to check, user-check: yes).
Next: the owner judges the look (user-check: yes); the noon azimuth stays
225 until then (021's note).
Regression (shadows off, the pinned test config): the same 13 of 16 as the
pre-existing baseline of task 054 (klacanyn-e0, klaratcv-e0 and
klatoxcv-e2 at 0 changed pixels), so the shadows-off view is exactly as
before wherever the references are valid.

### 2026-10-23 glm
Done: recorded the owner's review notes on the sunset shots (his screenshot
of the brahmin pen), no code changes.
Next: three follow-ups from the owner. 1) Strange dark shapes on the shaded
ground (the corrals shot): find the cause, whether the painted floor shadows
double the real ones, the sprite dimming or the bias. 2) A shadow stays
sharp but fades out at its border, a gradient falloff instead of a hard
step. 3) Smooth the stair-stepping along the shadow borders (a softened
lookup, PCF or the like). Claude works out the approach.
Check: the owner's chat screenshot; after the fix, sunset captures at the
corrals again, on and off.
