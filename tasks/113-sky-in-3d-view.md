---
id: 113
title: A sky above the 3D view instead of black
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
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
Done: Ported the donor-attributed full-screen sky pass with time-blended
colors, the sun glow, night colors and distant ridges. Underground maps keep
the dark donor color. The dawn/dusk windows are bounded so midnight stays
night instead of being classified as dawn.

Evidence: matching 1920x1080 before/after frames use separate executables and
scene caches. Main is `a8591799`; branch is `91e79b34`. Day, evening, night,
and cave pairs are in `runs/113-sky-in-3d-view/before-after-*-verified.jpg`;
safe metadata is in the adjacent JSON files. The Klamath fly tour changed 19
of 24 spots: all 19 Klamath views changed, while cave views stayed unchanged.
Its `changed.jpg` is attached below.

Check: Release build passed; CTest passed 18/18 on fresh main and branch.
Regression against fresh `origin/main` `a8591799` passed 16/16 with zero
changed pixels. All captures used explicit scene caches. `changed.jpg` and
four before/after JPEGs are below 1 MB each.

Environment note: The main checkout's `game/mod-cache/scene/kladwtwn/0`
contains 11 cache files dated 2026-10-29 10:02. This task did not alter them.
No verified pre-change copy or hashes are available, so their prior contents
and safe restoration cannot be confirmed.
Next: None.
