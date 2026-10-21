---
id: 002
title: Split the scene code into modules
milestone: M0
status: done
owner: claude
suggested: claude
claude-only: yes
user-check: no
depends-on: [003]
reviewed: yes
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

### 2026-10-19 claude
Done: `scene_gl.{h,cc}` is split into `context`, `scene_shader`, `floor`,
`walls`, `camera`, `live_objects`, `picking`, `scene_draw` with public headers
`context.h`, `camera.h`, `static_scene.h`, `live_objects.h`, `picking.h`,
`scene_draw.h`; shared state in `scene_internal.h` (each variable owned by one
module). Function bodies moved unchanged. Builds.
Next: regression on this build; then move the bridge from `devtools/` to
`ce/src/scene/`.
Check: `tools/regression.ps1` from this worktree.

### 2026-10-19 claude
Done: the game-to-scene bridge moved from `devtools/` to `ce/src/scene/`:
`scene_view` (which view is on, loading the map's scene, when it may be
presented, the per-frame refresh), `scene_objects` (live objects and the hero
into the scene, picking objects), `scene_overlay` (the game's picture over the
scene), `preview_policy` (moved, with its test). `devtools/` keeps the panel,
launch flags, snapshots, the mouse probe and the frame-rate log
(`devtools::scenePresented`, `devtools::sceneOverlaySynced`). The renderer's
`init` is now in `scene_renderer/context.cc` (task 004 edits it there).
Largest file in `scene/` and `scene_renderer/`: 294 lines.
Check: build and `ctest` pass (4 suites); `tools/regression.ps1`: all six spots
0 changed pixels after each step; `tools/dev-snapshot.ps1 -Probe`: 9 hit,
3 hidden behind walls, walk to 25875, talk opens the dialog, as before.
Rebased onto task 004 (#3): its OpenGL 4.1 context setup is ported into
`scene_renderer/context.cc`; build, `ctest` and the six regression spots pass
again (0 changed pixels).
