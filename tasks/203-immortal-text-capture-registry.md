---
id: 203
title: Leak the text capture registry so the exit-time free hook cannot touch destroyed statics
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

# 203: Leak the text capture registry statics

## Goal

Every `internal_free` runs the text capture hook
(`textCaptureAllocationDestroyed`), which uses function-local statics:
`ranges()` (a map), `layer()`, `captureMutex()`. While the process exits,
those statics are destroyed BEFORE the teardown frees stop coming, so the
hook then walks a destroyed map and a destroyed layer: use-after-free,
double frees, heap corruption, and access violations on the exit path
(observed in `textCaptureAllocationDestroyed+0x72` right after
`configFree`, and downstream as `dfileClose+0x18` on a mangled stream).
Kills the game the moment the player quits (reported as "loading a save
killed the game"; the crash landed at exit cleanup).

## Context

- Found 2026-11-06 in the owner demo session: AV 0xC0000005 at
  `textCaptureAllocationDestroyed+0x72` on the exit path, plus a second
  independent AV at `dfileClose+0x18` in a headless run that exited
  normally.
- The fix is a rule for the whole kind: state reachable from the memory
  free hook must not die at static destruction. The three accessors above
  leak their singletons deliberately; the OS reclaims the memory.

## Acceptance

- [x] A headless run that exits cleanly leaves no WER AppCrash record
  (before: every run could die in exit cleanup).
- [x] Build and tests green on CI.

## Out of scope

- A full audit of every function-local static in the codebase; only state
  reachable from the free hook is immortalized.

## Log

### 2026-11-06 gpt
Done: ranges()/layer()/captureMutex() leaked via new'd singletons. Demos of
the exit AV in the crash dumps resolve into the destroyed statics.
Next: CI, merge.
Check: a dev-snapshot run exits, then `Get-WinEvent` shows no fresh
Application Error for the test exe.
