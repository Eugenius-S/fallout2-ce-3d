---
id: 020
title: Game light sources as real lights
milestone: M1
status: todo
owner: none
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
