---
id: 021
title: Sun by game time and building shadows
milestone: M1
status: todo
owner: none
suggested: claude
claude-only: yes
user-check: yes
depends-on: [013, 020]
reviewed: no
---

## Goal

A sun that moves across the sky with game time and casts sharp shadows from
buildings, roofs and solids; characters darken in shadow.

## Context

- Game time comes from the engine; it jumps on waiting, travel and save load.
- Walls, roofs and furniture solids are geometry; character sprites cast no
  shadows.
- What a hidden roof does to shadows is decided in task 013.

## Acceptance

- Sun direction, height and color follow game time: long shadows morning and
  evening, no sun at night, no sun inside interiors. Time jumps do not flicker.
- Sharp shadows that follow open doors, hidden roofs and changed objects;
  an adjustable slight pixelation of shadows in the original graphic style,
  stable while the camera and the sun move.
- A sprite whose anchor is in a building's shadow darkens as a whole, keeping
  ambient light and other lamps, with a short transition at the border.
- `[3d] sun`, `[3d] shadows`, `[3d] shadow_pixel_size` with menu labels.
- Snapshots at noon, sunset and night; a character inside and outside a
  shadow; the frame time with shadows on and off in the `Log`.

## Out of scope

Cloud shadows (023).

## Log
