---
id: 004
title: OpenGL 4.1 core context
milestone: M0
status: todo
owner: none
suggested: gpt
claude-only: no
user-check: no
depends-on: [003]
reviewed: no
---

## Goal

The renderer runs on the context macOS supports, so nothing written from now
on needs porting later.

## Context

`ce/src/scene_renderer/scene_gl.cc` (`init`, around line 576) asks SDL for a
3.3 core context without the forward-compatible flag that macOS requires. GL
functions are loaded through our own list in `scene_renderer/gl_procs.h`.
Decided ceiling: OpenGL 4.1 core, no compute shaders, no SSBOs.

## Acceptance

- The context is 4.1 core with `SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG`; the
  log prints the version and renderer string once at start.
- If 4.1 is not available, the game falls back to the 2D picture with a log
  line and does not crash.
- No deprecated calls remain (check under a debug context: no GL errors in a
  regression run).
- The regression snapshots pass with the old references.

## Out of scope

The macOS build itself (M4).

## Log
