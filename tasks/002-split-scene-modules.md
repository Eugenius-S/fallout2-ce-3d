---
id: 002
title: Split the scene code into modules
milestone: M0
status: doing
owner: claude
suggested: claude
claude-only: yes
user-check: no
depends-on: [003]
reviewed: no
---

## Goal

Small modules with small headers, so a task touches one or two of them and
agents do not trip over each other.

## Context

`ce/src/scene_renderer/scene_gl.cc` (~1800 lines) holds the GL window, floor,
walls, live objects, picking, overlay and snapshots. `ce/src/devtools/devtools.cc`
(~2000 lines) holds the dev panel and launch flags but also the real
game-to-scene bridge: object cards, the mouse and interface overlay
(`syncSceneOverlay`, `maskSceneOverlay`, `scenePickObject`, `addObjectCard`).

## Acceptance

- The bridge lives in `ce/src/scene/`; `devtools/` keeps dev launch, panel,
  snapshots and the mouse probe only.
- `scene_renderer/` is split by purpose: context and present, floor and walls,
  live objects, picking, camera, and empty-ready places for light and post
  effects only when a later task needs them (no placeholders now).
- No source file in `scene/` or `scene_renderer/` over ~600 lines.
- Behaviour unchanged: the regression snapshots (task 003) pass with the old
  references; `-Probe` passes as before.

## Out of scope

Any change of behaviour or look.

## Log
