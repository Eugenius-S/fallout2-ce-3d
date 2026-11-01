---
id: 142
title: A dusty post-nuclear desert sky
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

The sky above the 3D view reads as a dusty, washed-out desert sky after the
war, not a clean saturated blue. No stars in daylight, and the far ridges
read as distant land, not soft orange waves.

## Context

- The owner, 2026-11-01 (`runs/inbox/2026-11-01-sky-garish.webp`, Klamath,
  daytime, fly camera looking up): "the sky in the 3D view looks garish;
  make it more desert and post-nuclear in colour", "and some kind of
  waves".
- What the shot shows:
  - the zenith is a bright saturated blue, the horizon a clean pale
    yellow: a modern postcard sky, not Fallout 2;
  - star specks cover the sky in full daylight and stretch into short
    streaks toward the horizon;
  - the two mesa ridges are smooth orange humps with blurred tops, which
    the owner reads as waves.
- The code: `ce/src/scene_renderer/sky.cc` (113). The colours are the
  `zenith`, `horizon` and `mesa` arrays blended by `day`, `twilight` and
  `night`; the stars are `uStars` (the `night` factor) in the fragment
  shader; the ridges are `uMesa` from noise over the compass direction.
- Fallout 2's palette for reference: dusty ochre, rust, faded khaki and
  grey-brown. The sky may keep a faint, desaturated blue high up, but the
  whole of it sits closer to a hazy beige-grey than to blue.

## Acceptance

- Day, dusk and night each read as a desert sky after the war: low
  saturation, a dusty horizon haze, no saturated blue anywhere.
- No stars while the sun is up; at night the stars are points, not
  streaks.
- The far ridges read as distant land: darker than the haze, with a
  harder top line, or removed if no general shape reads as land. No
  smooth blobs.
- The colours sit as named constants in one place in `sky.cc`.
- Evidence in `runs/142-desert-sky/`: fly-camera shots looking up and at
  the horizon over kladwtwn at 08:00, 12:00, 17:30 and 23:00, before and
  after, side by side.
- The sky tests and the scene tests pass.

## Log

- 2026-11-01 Cut from the owner's live find.
- 2026-11-01 gpt
  Done: named the day, twilight and night palette values in one place in `ce/src/scene_renderer/sky.cc`; removed daytime stars, made nighttime stars point-shaped, and changed the far ridge to stepped land silhouettes.
  Evidence: before/after sky-up and horizon pairs for Klamath at 08:00, 12:00, 17:30 and 23:00 are in `runs/142-desert-sky/`.
  Check: Release build passed (final executable SHA-256 `0ddc2032935ba71476973b1f62a8ea6608618bd74717ea790d4382f10f7cf05b`); 18 scene-related C++ test executables passed, including `sun_tests`. The Klamath fly tour passed at 57 views per side; its changed sheets are in `runs/142-desert-sky/fly-tour/`. `ctest -R "sky|scene"` found no registered CTest tests, and the repository has no sky-specific test executable.
- 2026-11-01 gpt: Owner rejected the dark stepped silhouettes. Removed the procedural ridges completely, leaving the dusty sky gradient. The previous horizon pairs and fly-tour sheets show the superseded silhouette; see `horizon-no-ridges.jpg` for the current result. Release build and CTest 18/18 passed. A new fly tour was attempted but stopped before its first capture because Windows PowerShell could not load Get-FileHash; rerun it with a working PowerShell environment before visual acceptance.
- 2026-11-01 gpt: The owner requested a black background in the normal 2.5D view at every rotation. The turn camera shares the fly camera implementation, so the sky is drawn only when flying and the turn camera does not own the pose. Captures at 90, 180, and 270 degrees show black outside the map; a free-flight capture retains the sky. See `black-background-rotations.jpg`. Release build and CTest 18/18 passed. A zero-degree capture at extreme zoom showed a black frame without visible map geometry, so it is not visual evidence. The full fly tour remains outstanding as noted above.
