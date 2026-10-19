---
id: 009
title: Regression snapshots at a fixed screen size
milestone: M0
status: doing
owner: gpt
suggested: gpt
claude-only: no
user-check: no
depends-on: [002, 003]
reviewed: no
---

## Goal

Regression results do not depend on the owner's window settings or monitor.

## Context

From the review of task 003. The references are 2310x1298 because
`tools/dev-snapshot.ps1` runs the test instance with `game/fallout2.cfg`, which
`tools/dev-launch.ps1` writes from this monitor's work area. After Ctrl+F11
(21:9) or a `-FullScreen` launch every spot fails on image size, and
`-FullScreen` (`windowed=2`) makes `dev-snapshot.ps1` refuse to run. On another
machine the references cannot match at all.

## Acceptance

- The background test instance takes its screen size, scale and windowed mode
  from dev flags (for example `--dev-screen=1920x1080 --dev-scale=2`), not from
  `fallout2.cfg`, and never writes the config.
- `tools/regression/spots.json` holds the regression's screen size and scale
  (a common 16:9 size at x2); references are taken again at it.
- Checked: switching the owner's config between 16:9, 21:9 and full screen
  does not change the results; the `Log` says how it was checked.
- 2D (vanilla framebuffer) snapshots are right at any width: today
  `takeSnapshot` passes the padded `gSdlSurface` rows to a tightly packed PNG
  writer, so an odd width (1155) gives diagonal artifacts (found in task 004).

## Out of scope

Changing how the owner's own game window is sized.

## Log

### 2026-10-19 gpt
Done: claimed after task 026 merged as PR #5 (`0fe4034`). Task 002 is also
merged; its dependency is now explicit. The existing GPT worktree is reused
for its build cache, without switching the shared primary checkout.
Next: parse dev-only screen/scale overrides before video initialization,
force background snapshots to windowed mode without config writes, fix the
vanilla surface pitch, then retake and compare all six references at 1920x1080
and scale 2. Verify alternate owner configs through isolated test config data.
Check: `tools/tasks.ps1`; task branch `task/009-regression-fixed-screen`.

### 2026-10-19 claude (note for gpt)
Task 011 added 10 regression spots (16 in all) and made 2D snapshots go
through the GL presentation when the renderer runs (`captureFramePng`), so
the odd-width problem is left only for runs without the GL renderer. Retake
all 16 references at the fixed screen size.

### 2026-10-19 claude (note for gpt)
Seen once after task 012's framing change: `klacanyn-e0` gave "No snapshot
within 90 s" (the capture, not the pixels); two reruns passed. If it comes
back, find where the still snapshot stalls on that map before raising the
timeout.

### 2026-10-19 gpt
Done: Added early dev-only fixed screen parsing, windowed settings override,
and guards for config migration and saves. The snapshot launcher defaults to
1920x1080 at scale 2; regression dimensions live in `spots.json`, and the
checker validates image size, logical size, scale and windowed mode even on
`--update`. Added parser and checker tests.
Next: After review, fix the vanilla framebuffer pitch for odd widths, then
verify fixed-size captures against isolated config fixtures and retake the 16
references.
Check: Release build succeeded; `ctest` passed 6/6; exporter tests passed 36
and regression tests passed 12. No runtime captures were run in this step.
