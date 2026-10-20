---
id: 031
title: A free-flying camera for looking around maps
milestone: M1
status: todo
owner: none
suggested: claude
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A developer flies the 3D camera freely over a map to look for flaws up close
and to record videos, as in the donor viewer.

## Context

- The renderer already draws the scene from any eye: the fixed preview
  camera (`StaticPreviewCamera`, `makeFloorMvp`) is a plain look-at camera.
  The game view (`game_view.h`) is the default.
- Dev hotkeys live in `ce/src/devtools/devtools.cc`. Ctrl+F11 is taken (the
  window shape 16:9 / 21:9); pick a free key and list it in `docs/p1-devtools.md`.
- In fly mode the game must not get the keys and the mouse (no walking,
  no hotkeys); the game itself may keep running or pause.
- Sprites are upright cards facing the game's camera, so from the side they
  look like cardboard: fine for this view.

## Acceptance

- One key (with `--dev-tools`) enters and leaves fly mode; leaving returns
  the game view exactly as before.
- WASD moves, the mouse looks around, Q / E go down / up, Shift is faster;
  the movement is smooth enough for a video at 60 fps.
- A dev snapshot works in fly mode.
- Not available without `--dev-tools`.

## Out of scope

Recording video inside the game (an outside recorder is used).

## Log
