---
id: 104
title: The 3D camera goes a little farther at the map's bottom edge, so the exit zone shows
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: [095]
reviewed: yes
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

### 2026-10-28 gpt: bottom edge

- The 3D EDG bottom limit extends by 24 map pixels (one hex-row step). At the owner's Klamath exit the visible green zone moves above the interface. A 48-pixel trial exposed a larger black gap above the interface, so the smaller step was chosen. A narrow black strip remains at this map boundary; the existing map clip was not changed.
- With the same start and hero tiles, Klamath's bottom center changed 28108 to 28509; Grazing Grounds changed 24546 to 24947. Klamath left/right/top centers stayed 25337/28108/20100. Classic bottom centers and both Classic framebuffer pairs stayed identical (0 changed pixels).
- `runs/104-camera-bottom-edge-farther/` contains side-by-side game screenshots, including the interface. The captures were made with the same scene cache at 1920x1080, scale 2. CTest passed 18/18.
- Direct comparison against current main: all 16 regression scenes match exactly in RGBA with the same capture metadata (0 changed pixels). The stored-reference checker reports the same five failures on both builds: KLAGRAZ e0 183 px, KLARATCV e0/e1/e2 3837/5185/6299 px and KLATOXCV e2 550 px; no references were changed. Exporter tests 66/66 and regression unit tests 12/12 passed.
