---
id: 113
title: A sky above the 3D view instead of black
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Where the 3D view (flight and the turned camera) looks over the map's edge
or up past the walls, it shows a sky, not black. The sky follows the time
of day the scene already lights by.

## Context

- The owner, 2026-10-28: the first-person port
  (https://github.com/Apolotary/fallout2-first-person, our donor) has a sky
  and the 3D view looks better with it
  (`runs/inbox/2026-10-28-sky-reference.webp`). Today the background is the
  clear colour (`ce/src/scene_renderer/scene_draw.cc`).
- The donor's sky is one full-screen pass drawn first, a function of the
  view ray only: `viewer/render.js`, `SKY_FS` (horizon haze, a gradient to
  the zenith, the sun's glow, stars at night, two far mesa ridges from
  noise over the compass direction) and the colours in `MOODS` (day, dusk,
  night) and `underground()`. Port that, keep the donor's attribution as
  for the exporter.
- Our sun: `ce/src/scene/sun.h` / `sun.cc` (direction and colour by game
  time). The sky's sun glow sits on that direction; the colours blend by
  the same game time, not by the donor's three fixed moods.
- Underground maps (the caves, indoor maps without daylight) get no sky:
  the donor's dark `underground()` colour.

## Acceptance

- before | after in flight over Klamath at three game times (day, dusk,
  night) and in a cave, in `runs/113-sky-in-3d-view/`.
- The original game view is untouched: the regression stays pixel for
  pixel; ctest passes.

## Out of scope

Clouds and their shadows (023/069), fog on the scene, a moon light.

## Log

### 2026-10-29 gpt
Done: Added the donor-attributed full-screen sky pass for flight and turned 3D cameras, with time-blended colors, sun glow, night stars, distant ridges, and the donor's dark underground color. The pass skips the empty-floor fill in flight so the sky remains visible beyond the map boundary. Final after captures are in `runs/113-sky-in-3d-view/`.
Next: Capture matching before views from an untouched baseline, diagnose the five pixel-regression failures, and resolve the existing `settings_3d_tests` failure before completing acceptance.
Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release` passed. `ctest --test-dir build --output-on-failure` had 17/18 passing; `settings_3d_tests` failed on `missing clean_edges key uses its literal true default`. Final `tools/regression.ps1` had 11/16 spots pass and 5 fail: entrance 191092 pixels, map-edge 453853, klacanyn-e0 246157, klagraz-e0 42372, klaratcv-e2 6299. Other 11 spots passed, with 0 changes in 9, 3 in klatoxcv-e0 and 1 in klatrap-e0. Before captures were not obtained because the baseline build had been replaced before capture began.
Environment note: The main checkout's `game/mod-cache/scene/kladwtwn/0` contains 11 cache files dated 2026-10-29 10:02. This task did not alter them. No verified pre-change copy or hashes are available, so their prior contents and safe restoration cannot be confirmed.
