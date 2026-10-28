---
id: 113
title: A sky above the 3D view instead of black
milestone: M1
status: todo
owner: none
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Where the 3D view (flight and the turned camera) looks over the map's edge
or up past the walls, it shows a sky, not black. The sky follows the time
of day the scene already lights by.

## Context

- The owner, 2026-10-28: the first-person port
  (https://github.com/Apolotary/fallout2-first-person, our donor) has a sky
  and the 3D view looks better with it
  (`runs/inbox/2026-10-28-sky-reference.webp`). Today the background is the
  clear colour (`ce/src/scene_renderer/scene_draw.cc`).
- The donor's sky is one full-screen pass drawn first, a function of the
  view ray only: `viewer/render.js`, `SKY_FS` (horizon haze, a gradient to
  the zenith, the sun's glow, stars at night, two far mesa ridges from
  noise over the compass direction) and the colours in `MOODS` (day, dusk,
  night) and `underground()`. Port that, keep the donor's attribution as
  for the exporter.
- Our sun: `ce/src/scene/sun.h` / `sun.cc` (direction and colour by game
  time). The sky's sun glow sits on that direction; the colours blend by
  the same game time, not by the donor's three fixed moods.
- Underground maps (the caves, indoor maps without daylight) get no sky:
  the donor's dark `underground()` colour.

## Acceptance

- before | after in flight over Klamath at three game times (day, dusk,
  night) and in a cave, in `runs/113-sky-in-3d-view/`.
- The original game view is untouched: the regression stays pixel for
  pixel; ctest passes.

## Out of scope

Clouds and their shadows (023/069), fog on the scene, a moon light.

## Log
