---
id: 004
title: OpenGL 4.1 core context
milestone: M0
status: done
owner: gpt
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

### 2026-10-19 gpt
Done: claimed task 004 after task 003 merged as PR #2 (`bb64a69`). Read the
current init/loader/fallback paths. The renderer still requests 3.3 core,
does not request forward compatibility and does not log context identity.
Worktree: `E:/Projects/fallout2-ce-3d-gpt`; branch: `task/004-gl41-core-context`.
Claude owns task 005 in its separate worktree; do not take it or switch the
shared game checkout away from main.
Next: request and verify 4.1 core plus forward compatibility, log the actual
version/renderer once, test debug-context errors and unavailable-context
fallback, then run the six unchanged references from task 003. Build in this
worktree; runtime checks must use `tools/dev-snapshot.ps1` in the shared game
checkout, one background instance at a time. No implementation changes yet.
Check: `git show origin/main:tasks/004-gl41-core-context.md`; inspect
`ce/src/scene_renderer/scene_gl.cc` (`init`) and `gl_api.cc` (`glLoad`).

### 2026-10-19 gpt
Done: rebased onto current origin/main. The context now requests and verifies
4.1 core plus forward compatibility, logs its identity in `scene-context.log`
and resets SDL GL attributes on failure before the vanilla renderer starts.
`F2CE_GL_DEBUG=1` requests a debug context and enables optional 4.1-compatible
ARB_debug_output diagnostics. Release build and all 43 unit checks pass.
All six old references pass with zero changed pixels under OpenGL 4.1.0,
profile 0x1, flags 0x3; six callbacks enabled and no errors/deprecations logged.
Evidence: `runs/004-gl41-core-context/`.
Next: prove the unavailable-context path with a temporary impossible context
request and an in-process F9 event, restore/rebuild, then PR and merge promptly
so Claude can rebase task 002. Local proof driver:
`powershell -NoProfile -ExecutionPolicy Bypass -File build/task004-fallback-proof.ps1`.
Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/regression.ps1`;
inspect `runs/004-gl41-core-context/debug-context.log`.

### 2026-10-19 gpt
Done: every acceptance point passes. Six unchanged references match exactly
under a verified 4.1 core forward-compatible debug context; ARB diagnostics
reported no GL errors or deprecated calls. An impossible 99.1 request proved
the live fallback: a background F9 snapshot shows KLADWTWN through
`sdl-renderer` / `vanilla-framebuffer`, with no crash. The F9 event was posted
inside only that test process, never to a desktop window. Temporary source
changes were restored byte-for-byte and the restored 4.1 entrance again
matched the old reference. Evidence: `runs/004-gl41-core-context/README.md`.
Next: merge the PR promptly for Claude's task 002 rebase, then explicitly
claim task 026 as the owner requested. Task 009 waits for task 002.
Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/regression.ps1`;
set `F2CE_GL_DEBUG=1` to collect optional ARB error/deprecation diagnostics.
