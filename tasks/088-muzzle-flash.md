---
id: 088
title: A shot flashes at the gun's muzzle, one flash per bullet in a burst
milestone: M1
status: todo
owner: none
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: [086]
reviewed: no
---

## Goal

When someone fires a gun, a flash of light blinks in front of them at the
muzzle. A burst gives several flashes, one per shot. Each flash lands on the
frames where the shooter's own sprite brightens at the shot, so the light and
the picture blink together.

## Context

- The owner, 2026-10-26: the next stage of the glow work after 086 (gpt's
  self-lit weapon effects, local colored light and bloom). The flash uses
  the same three layers: a bright core, a short local light on what stands
  near, and the bloom, under the one global glow strength.
- Timing and place come from the art, not from a hand-made table: the
  firing animations (single shot and burst) already paint the muzzle flash
  as bright pixels near the barrel on some frames. Find those frames and the
  bright cluster's position per frame and rotation; the flash blinks on
  those frames at that spot. A burst's repeated bright frames give its
  repeated flashes.
- Energy weapons (laser, plasma, pulse and the rest): the bolt leaving the
  barrel is already self-lit and casts its light by 086, so the muzzle
  glows by itself. If that reads too weak at the muzzle, add a smaller
  flash there, with the bolt's colour; show both, with and without it, and
  let the owner pick.
- The flash is visual only: no change to combat, timing, sound or the
  game's own frames.

## Acceptance

- Night sheets in `runs/088-muzzle-flash/`, laid out in rows: one row per
  weapon and rotation, the firing frames left to right in order, each frame
  labelled with its number; under each row the same frames without the
  flash. The flash sits on the same frames the sprite brightens.
- Rows for: a pistol, a rifle, a burst weapon (every shot of the burst
  visible), a laser and a plasma weapon (their own glow, and with the
  smaller flash added); each at least at two rotations.
- Glow off: no flash; the original view regression stays pixel for pixel.
- JPEG sheets at most 1 MB each.

## Out of scope

New art, weapon balance, projectile trails, per-weapon menu controls.

## Log
