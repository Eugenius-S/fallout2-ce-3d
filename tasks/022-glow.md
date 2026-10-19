---
id: 022
title: Glow on fire, screens, signs and energy weapons
milestone: M1
status: todo
owner: none
suggested: gpt
claude-only: no
user-check: yes
depends-on: [002, 014]
reviewed: no
---

## Goal

Things that shine in Fallout 2 glow: fire, monitors, alarms, signs, laser and
plasma shots.

## Context

- The game marks most shining pixels already: the palette's cycling colors
  (indices 229-254 in `ce/src/cycle.cc`: slime, shoreline, slow fire, fast
  fire, monitors, alarm). Decide per range which glow (shoreline likely not).
- Sprite frames become RGBA card textures in the bridge; floor and wall
  textures come from the exporter. The glow mask has to be made where the
  palette index is still known: at both places.
- The same colours stay at full light in the engine's darkening. Sprite cards
  already mark them (alpha 254, task 020); floor and wall textures from the
  exporter do not yet, so in caves and at night they dim there: the
  exporter's mask fixes that too.
- Energy weapon shots and explosions: check which colors their art uses; if
  they are not cycling colors, add their palette colors to the mask list.
- Post effects stay within OpenGL 4.1 core.

## Acceptance

- A glow (bloom) pass: bright parts bleed light softly; colors keep cycling
  as in the game.
- Klamath: fires, monitors and signs glow; a laser shot glows (test on any
  map with a laser fight, snapshots in the PR).
- `[3d] glow` (default on) and `[3d] glow_strength` with menu labels.
- Regression snapshots with glow off unchanged.

## Out of scope

Light that glowing pixels cast on surroundings (task 020 covers light).

## Log
