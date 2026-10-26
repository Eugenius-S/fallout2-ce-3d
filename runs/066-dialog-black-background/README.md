# Dialogue backdrop verification

The dialogue now owns a full-screen opaque black window below its existing
windows. Destroying it refreshes the exposed screen synchronously through
`windowDestroy`, with no timer or deferred cleanup. The dialogue art, text,
barter windows and cursor keep their existing composition.

## Captures

- `dialogue-owner.jpg`: 2310x1298, scale 2, Russian, sharp text enabled.
- `dialogue-1080.jpg`: 1920x1080, scale 2, the same Sulik dialogue.
- `street-after-goodbye.jpg`: the same 1080p process after choosing option 7
  (the goodbye reply) through window messages sent only to that helper PID.
- `verification.json`: framebuffer bounds and executable hashes.

The raw PNG captures stay in the local build and game screenshot folders.
Their pixels outside the dialogue rectangle are all RGB (0, 0, 0):
`[514,68,1794,1028)` at the owner's resolution and
`[320,60,1600,1020)` at 1080p. The first owner-resolution before/after pair
has zero changed pixels inside the dialogue rectangle; before the fix,
1,763,236 pixels outside it were nonblack.

The stock snapshot copier expects vanilla screenshots beside their JSON,
but those PNGs currently go to `game/shots/game/`. A temporary local copy
of the harness resolved that existing path mismatch. The goodbye harness
also used a distinct helper executable name and PID-scoped window messages;
it never focused, stopped or modified the owner's game.

The fixture now waits for dialogue options before accepting a frame and
permits a successfully captured dialogue to return normally. This avoids
mistaking world text during initial backdrop composition for a ready
dialogue, or reporting a normal goodbye as a failed window opening.

## Checks and regression diagnosis

Release build, 15 CTests, 52 exporter tests and 12 regression unit tests pass.
The complete 16-spot visual regression has 3 passes and 13 reference mismatches.
A control build with both changed engine files restored to base `04dbdf88`
produces exactly the same images as the patched build at all 16 spots. Strict
comparison allows no channel tolerance and reports 0 changed pixels everywhere.
Every old-reference mismatch also reproduces in the control. Per-spot counts,
validation settings and executable hashes are in `baseline-comparison.json`.

The ordinary regression checker passes 16/16 against the temporary current-base
reference set. The committed references still come from 2026-10-22, source
`32f0fe2b`, and were not updated. Their independent audit is task 084. This task
changes no world pixels relative to its base.
