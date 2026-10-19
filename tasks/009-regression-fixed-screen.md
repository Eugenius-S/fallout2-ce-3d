---
id: 009
title: Regression snapshots at a fixed screen size
milestone: M0
status: done
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
vanilla surface pitch, then retake and compare all 16 references at 1920x1080
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

### 2026-10-19 gpt
Done: The vanilla SDL snapshot path now copies rows using the surface pitch before PNG encoding; its production-helper test checks all pixels at width 1155 with 5 padding bytes per row. `dev-snapshot.ps1 -ConfigDirectory` uses a copied test executable and isolated `fallout2.cfg`/`ddraw.ini` in `build/task009-*`, while the main game directory remains the resource working directory. Captures from 16:9, 21:9 and fullscreen fixtures produced identical decoded RGBA pixels. Each reports 1920x1080 drawable, 960x540 logical, scale 2 and windowed mode. Owner `fallout2.cfg` remains at SHA-256 `875111504363B0A54B63002919598B175E64DECF9E84C5F5A67BC915291FE927`.
Next: Parent review Step2; then retake all 16 regression references at 1920x1080 and scale 2 after authorization.
Check: One clean Release rebuild (to avoid inherited stale MSVC/Ninja header dependencies) passed; CTest 7/7, exporter 36 and regression 12; padded-row standalone test 1/1. Three background captures used tile 25075; their decoded 1920x1080 RGBA buffers matched byte-for-byte. Fixture config timestamps predate captures and final hashes were recorded; no test process remains.

### 2026-10-19 gpt
Done: Confirmed the intermittent capture stall can be caused by the SDL focus-loss pause path: an in-process synthetic `SDL_WINDOWEVENT_FOCUS_LOST` was processed and entered `_GNW95_lost_focus`, then no snapshot appeared within 20 seconds. Added a narrow exception for fixed-screen background captures only; normal/manual focus handling remains unchanged. Repeating the same synthetic event now completes and writes a real PNG/JSON. Temporary event injection/logging was removed, `input.cc` was rebuilt for game and mapper, and the final source/EXEs contain no probe marker. The regression launcher now records stdout/stderr per spot.
Next: Retake and compare all 16 references with the updated capture runner; document any failed batches and preserve the old references unless the entire update validates.
Check: Release incremental build succeeded; CTest 7/7, exporter 36/36, regression 12/12. Focus failure/success logs and successful capture are under `runs/009-regression-fixed-screen/synthetic-focus-loss*`. Two earlier full updates failed atomically at `klamall-e0` and `klatrap-e0`; isolated same-map captures succeeded. Their actual historical focus state was not observed, so the reproduced focus pause is a proven cause for possible stalls, not a claim about those two events.

### 2026-10-19 gpt
Done: Replaced all 16 regression PNG/JSON references atomically at 1920x1080, logical 960x540, scale 2 and windowed mode. The following full comparison passed all 16 unchanged references with 0 changed pixels each. Fixture proof records the 16:9, 21:9 and fullscreen input config shapes and hashes; each produced identical decoded RGBA pixels while the owner config hash remained unchanged. A before/after entrance pair documents the intentional size change from 2310x1298 to 1920x1080. Earlier failed update runs at `klamall-e0` and `klatrap-e0` were atomic; final successful run IDs `20261019-170723-951-65956` (update) and `20261019-171029-833-62892` (comparison) are summarized under `runs/009-regression-fixed-screen/`.
Next: Parent final review and PR acceptance.
Check: Full `tools/regression.ps1 -Update` passed all 16; subsequent `tools/regression.ps1` passed all 16 with 0 changed pixels. References and evidence metadata were validated at 1920x1080, logical 960x540, scale 2, windowed true. `tools/tasks.ps1` reports task 009 done; `reviewed: no` pending parent review.
