---
id: 104
title: The 3D camera goes a little farther at the map's bottom edge, so the exit zone shows
milestone: M1
status: todo
owner: none
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: [095]
reviewed: no
---

## Goal

At a map's bottom edge the 3D camera stops a little lower than now, enough
that the exit zone there is clearly in view.

## Context

- The owner, 2026-10-27, live with the 095 build: at the bottom edge the
  exit zone is barely visible
  (`runs/inbox/2026-10-27-bottom-edge-exit-zone.jpg`, the green zone at the
  bottom). 095 made the 3D camera stop exactly where the original does; the
  owner wants the bottom stop pushed farther, in the 3D view only.
- The same shot has a shadow flaw; that belongs to 096, not here.

## Acceptance

- The bottom stop moves down by a fixed amount (state it, and why it is
  enough) in the 3D view; the other three edges and the original view stay
  as 095 left them.
- Shots of the bottom edge of kladwtwn and one more map in both views, in
  `runs/104-camera-bottom-edge-farther/`.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

The other edges, the fly camera.

## Log
