---
id: 007
title: CI builds the game on Windows
milestone: M0
status: doing
owner: glm
suggested: glm
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Every push and PR shows whether the whole game still builds and its tests pass.

## Context

- `.github/workflows/verify.yml` runs the exporter tests and four scene unit
  test suites on Ubuntu, but never builds the game.
- `tools/configure-build.ps1` has paths to MSVC and the Windows SDK of the
  owner's machine hard-coded (that machine's VS install is partly broken, so
  the paths are needed there).
- The task needs no game files, so it can run in its own worktree next to
  another agent; the worktree builds into its own `build/`.

## Acceptance

- A Windows job in `verify.yml` builds `fallout2-ce.exe` in Release with the
  same CMake options as `tools/configure-build.ps1`, and runs `ctest`.
- `configure-build.ps1` falls back to the standard Visual Studio environment
  (vswhere / VsDevCmd) when the hard-coded paths are missing, and still works
  on the owner's machine unchanged.
- The workflow is green on a PR.

## Out of scope

macOS and Linux builds of the game (M4); anything that needs game files.

## Log

### 2026-10-19 glm
Done: tools/configure-build.ps1 falls back to the standard Visual Studio
environment (vswhere + vcvars64.bat) when the owner's hard-coded MSVC/SDK
paths are missing; the owner layout is unchanged. A `windows-game` job in
.github/workflows/verify.yml builds the game in Release through the script
and runs ctest.
Next: push the branch, open the PR, watch the workflow go green on the PR,
merge.
Check: the `windows-game` job of the Verify workflow on the PR.
