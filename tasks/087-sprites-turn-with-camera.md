---
id: 087
title: Characters turn with the camera: the sprite shows the side the camera sees
milestone: M1
status: todo
owner: none
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

When the camera turns, people, creatures and robots (every critter) show
the side of themselves the camera now looks at, so a flat sprite reads as
a round figure. First stage: all critters. Things drawn from all sides
that stay sprites come later.

## Context

- The owner, 2026-10-26: objects that have art for every side but stay
  sprites should turn as the camera turns, faking volume. Start with all
  characters; check the look on one, then roll it out to the rest without
  looking at each.
- Fallout draws a critter in 6 directions, 60 degrees apart (`rotation`
  0..5 in the frame). Today a card faces the fly camera
  (`faceFlyCamera` in `ce/src/scene_renderer/live_objects.cc`) but keeps
  the object's own `rotation`, so a turned camera still sees the side the
  game's camera would.
- The idea: the shown rotation is the object's rotation shifted by the
  camera's turn from the game's view, rounded to the nearest 60 degrees.
  This holds for the free fly camera and the turned game camera (038,
  limited to 45 degrees by 077). Near a 30-degree boundary keep a little
  hysteresis so the sprite does not flicker between two sides.
- The frame texture cache already keys on rotation (`objectArtKey`); a
  shown rotation that differs from the game's one is only another key.
- Game logic, facing and combat stay on the game's own rotation; only
  what is drawn changes.

## Acceptance

- One critter (the hero) shot at camera turns 0, 30, 45, 60 and 90
  degrees, before | after, in `runs/087-sprites-turn-with-camera/`, so the
  owner can judge the look.
- Every critter on the map turns the same way (one short sheet of a few
  critters is enough).
- The game's own view stays pixel for pixel (the regression).

## Out of scope

Scenery and items drawn from all sides (a later stage of this task or a
new one); real 3D figures.

## Log
