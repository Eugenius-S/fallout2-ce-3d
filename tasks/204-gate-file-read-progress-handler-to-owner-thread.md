---
id: 204
title: Invoke the file read progress handler only on the thread that installed it
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

# 204: Invoke the file read progress handler only on the thread that installed it

## Goal

`mapLoadSaved` installs `fileSetReadProgressHandler(gameMouseRefreshImmediately,
32768)` on the main thread. The handler pumps SDL events and presents a frame,
which is main-thread-only work. The 172 warm pool reads art files on worker
threads, and every worker that read an FRM through `fileRead` invoked the
handler off-thread: `gameMouseRefreshImmediately -> renderPresent ->
WIN_PumpEvents -> prepareSharpTextForPresent` ran concurrently with the main
thread's renderer state. Symptoms in the owner demo: silent hangs at map load
(the workers stuck inside `prepareSharpTextForPresent` /
`uploadSharpTextOverlay` while the main thread waited in `_Thrd_join`) and
`0xC0000374` heap corruption crashes during "load a save".

The rule: only the thread that installed the handler may invoke it. Every
other thread takes the plain read path. Four call sites invoke the handler
(`dbGetFileContents`, `fileReadChar`, `fileReadString`, `fileRead`); all four
are gated by the same owner-thread check, and `fileSetReadProgressHandler`
records the installing thread id.

## Context

- Found from a live dump of a hung `--dev-load-game=1` run (parallel warm
  pool): three worker threads parked in `prepareSharpTextForPresent` /
  `uploadSharpTextOverlay` reached through `artLock -> cacheLock -> artRead ->
  fileRead -> gameMouseRefreshImmediately`, main thread in `_Thrd_join`.
- The first fix attempt gated only three of the four invoke sites and changed
  nothing: the dump showed the workers enter through `fileRead`, the bulk read
  the art loader uses. Gating the fourth site is what fixed it.
- The pre-fix serial path (warm pool disabled) always loaded fine, which
  matched the owner's report: plain runs survived, "load a save" did not.

## Acceptance

- [x] A hidden `--dev-load-game=1` run with the parallel warm pool reaches the
  kladwtwn scene load and keeps running (3 of 3 runs; before the fix: 0 of 3,
  no output past the OpenGL init line).
- [x] The same run with the warm pool disabled still loads (2 of 2, no
  regression on the serial path).
- [x] Build and tests green on CI.

## Out of scope

- The 172 warm pool discipline itself.
- The progress handler's rendering work (a later task may move the handler to
  a main-thread queue instead of dropping it; today's rule simply skips it on
  worker threads, which is also what the vanilla binary effectively did with
  its single-threaded reads).

## Log

### 2026-11-07 gpt
Done: db.cc records `gFileReadProgressOwnerThread` in
`fileSetReadProgressHandler` and gates all four invoke sites with
`std::this_thread::get_id() == gFileReadProgressOwnerThread`. Verified in the
owner demo copy: parallel A/B went 0/3 -> 3/3 loaded, serial stayed 2/2.
Check: the hidden `--dev-load-game=1` A/B against the demo game dir prints
`loaded=True` three times with the parallel warm pool (machine-local
tooling, not committed).
Next: CI, merge, done.
