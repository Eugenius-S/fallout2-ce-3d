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
