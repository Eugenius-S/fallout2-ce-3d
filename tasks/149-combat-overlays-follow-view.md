---
id: 149
title: Combat outlines, the hex cursor and floating text follow the 3D turn and zoom
milestone: M1
status: todo
owner: none
suggested: claude
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

On a turned or zoomed 3D view, what the game draws over the map in combat
sits on the thing it belongs to: the friend and foe outline follows the
critter's shown sprite, the hex cursor and its to-hit number sit on the
hex, floating text above the head.

## Context

- 135 step 1 (#211) draws the combat outlines from the scene module, the 2D
  outline moved to the hex the scene shows (`scene_overlay.cc`
  `drawOutline`). Codex on #211: `scene_objects.cc::shownArt()` picks a
  camera-relative sprite side while the outline uses `object->fid` and
  `object->rotation` at the original size, so a turned view outlines
  another silhouette and a zoomed view an unscaled one.
- Build the outline from the shown art, with the card's projection and
  scale (in the renderer, as an edge pass on the sprite card, or by
  drawing the shown frame's outline into the overlay at the card's size).
  Check the hex cursor and floating text the same way at zoom 0.5 and 2 and
  turn ±30.

## Acceptance

- Captures at zoom 0.5, 1, 2 and turn 0, +30, -30 in combat (146's fixture
  when it lands, or a still with highlighting on) in
  `runs/149-combat-overlays-follow-view/`, outline on the sprite in each.
- The 2D picture and the unturned zoom 1 view look as before.
- ctest passes.

## Log

- 2026-11-01 Cut from Codex's review of #211.
- 2026-11-02 The owner, live: the outlines keep their size on a zoomed view (`runs/inbox/2026-11-02-combat-outlines-unscaled.png`).
