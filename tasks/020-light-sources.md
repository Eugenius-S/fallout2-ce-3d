---
id: 020
title: Game light sources as real lights
milestone: M1
status: doing
owner: claude
suggested: claude
claude-only: yes
user-check: yes
depends-on: [012, 014]
reviewed: no
---

## Goal

Lamps, fires and other light sources of the game light the 3D scene; sprites
brighten near them, darken away from them and take their color.

## Context

- The game's light: objects carry `lightDistance` and `lightIntensity`
  (`ce/src/obj_types.h`), tiles keep an intensity map and the map an ambient
  level (`ce/src/light.h`). Game visibility rules must not change: the 3D
  light is a picture, not a mechanic.
- Position, radius, brightness and on/off come from the game state; height,
  color and kind (lamp, torch, fire, neon) from the object.
- Decide whether 3D light replaces the game's 2D light in the picture or adds
  to it, by comparing both on Klamath at night and day; write the choice in
  the `Log`.

## Plan (claude, 2026-10-19)

- Today the 3D scene applies no light at all: night maps and caves show at full
  brightness, unlike 2D. First restore the game's own light as the authentic
  base: ambient (`lightGetAmbientIntensity`; map scripts set it by hour through
  `set_light_level`, `interpreter_extra.cc`), and the per-tile intensity the
  engine spreads from light objects (`lightGetTileIntensity`; objects with
  `lightDistance` up to 8 hexes and `lightIntensity` up to 65536). Floor and
  walls lit per tile, interpolated; sprites by their anchor tile. Snapshots
  must match 2D brightness at day and night.
- Then real lights: one point light per object with `lightDistance > 0`,
  live every frame (position, radius, intensity, on/off). Height and color
  from the exporter's `glows` (`scene.json`: u, v, px right, height, r, g, b;
  fires 1.0/0.5/0.18, lamps 1.0/0.86/0.6), matched by position; otherwise a
  default by object kind.
- The exporter's `light.png` (R lamps, G daylight, B floor shade) is a static
  bake: a reference picture, not a live source.

## Acceptance

- Every light-carrying object on Klamath maps lights floor, walls and sprites;
  switching a light in the game switches it in 3D.
- Sprites are lit at their anchor point, no normal maps; no flicker at the
  edge of a light.
- `[3d] lights` (default on) with a menu label; off gives today's look.
- Snapshots off and on at the same spots, day and night.

## Out of scope

Shadows from point lights.

## Log

### 2026-10-19 claude
Done: research; findings and plan in `## Plan`.
Next: start when the dependencies are done.
Check: read `## Plan`.

### 2026-10-19 claude
Done, step 1: the scene shows the game's own light, as the engine paints it.
Each frame the scene takes every hex's light (at least the ambient level) as
a 200 x 200 map: the floor is lit by the hexes under it, between them; walls
by their object's hex (walls.bin's object table); sprites and the hero by
their hex; roofs by the ambient level. The texel is darkened in the engine's
128 steps on its 5-bit colour, so dark places match 2D (cave: 0.93..0.99 of
the 2D brightness; Klamath at the darkest night: 13.0 against 13.6).
`[3d] lights` (default on, menu label "Light"): off shows the unlit scene.
New dev option `--dev-ambient` (`dev-snapshot.ps1 -Ambient 16384`) holds the
map's ambient light for night views.
Choice: 3D light replaces the 2D light in the picture; step 1 is the 2D light
itself, so nothing changes at day and caves and nights are as dark as in 2D.
Evidence: `runs/020-game-light/` (cave and Klamath at night, 2D | 3D).
The six cave references retaken; the town spots (day) are unchanged.
Next: step 2, real lights from light-carrying objects (height, colour, radius).
Check: `tools/regression.ps1` (16 pass); `ctest --test-dir build` (8/8).

### 2026-10-19 claude
Done: merged main with task 027. `[3d] lights` keeps only its own revision:
switching it neither changes the `[3d] enabled` revision nor switches the
scene (a direct test in `settings_3d`).
Next: merge PR #20; then step 2.
Check: `ctest --test-dir build` (8/8); `tools/regression.ps1` (16 pass).
