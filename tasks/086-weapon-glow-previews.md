---
id: 086
title: Native plasma, laser and explosion glow previews
milestone: M1
status: doing
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: [073]
reviewed: no
---

## Goal

Show the owner the game's native weapon effects with glow OFF and at the
accepted common default 2: laser, plasma variants and explosions.

## Context

The owner requested more effects after approving 073's monitor and slime.
The developer harness has no weapon-effect fixture yet. Use native misc art,
the real object/card renderer and 073's corrected frozen palette.

## Acceptance

- A bounded developer fixture places a native effect at a chosen tile/frame,
  without combat, damage, saving or touching the owner's game.
- Same-scene OFF/default-2 pairs for available laser/plasma and fire, plasma,
  EMP and rocket explosions, labelled with real asset/frame information.
- JPEG sheets under runs/086-weapon-glow-previews, at most 1 MB per file.
- Build/tests and ordinary glow-off regression remain green against controls.

## Out of scope

Weapon balance, real combat simulation, animation timing, new art, emission
mask changes, or changing the approved global glow default.

## Pipeline

INPUT: native misc FID and a representative fixed frame from the game data.
TRANSFORM: spawn a no-save object through the engine, freeze the scene/palette.
MEANING: the normal scene bridge and shader draw the actual native effect.
OUTPUT: two captures differing only by glow OFF versus the global default 2.
DIRECT CHECK: effect metadata/frame, matching scene state, pixel differences.

## Log
