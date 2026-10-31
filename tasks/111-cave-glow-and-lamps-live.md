---
id: 111
title: The slime pools glow and the lamps light the walls in the live caves
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

In the owner's live game, in the 3D view of the Klamath caves, the green
slime pools glow (022, 073) and the point lights light the walls near them
(099), as the snapshots show them.

## Context

The owner, 2026-10-28, live on main `9705aca4`:

- `runs/inbox/2026-10-28-slime-no-glow.png`: the toxic caves
  (klatoxcv), the slime pools are flat green, no glow round them.
- `runs/inbox/2026-10-28-caves-no-point-light.webp`: the toxic caves
  again; the owner noticed no dynamic light anywhere, though the caves
  look fine overall.
- The 2026-10-24 inbox line says the same of the burning barrels in
  Klamath. 073 found the snapshot fixture froze the palette cycle and
  fixed the slime there; the live game was not checked after it.

So the snapshots and the live game may differ: settings (the `[3d]`
section of the owner's ini against the snapshot's), the palette cycle in a
running game, or the scene cache the game loads.

## Direction

Reproduce in the live game's conditions first: the owner's ini
(`game/fallout2.cfg` and the mod ini, read only, never edit the owner's
files), a non-still run, the main checkout's cache. Find which condition
turns the glow and the lights off, then fix that cause, not the look.

## Acceptance

- The cause named in the Log, with what differs between the snapshot and
  the live game.
- before | after of the slime pools and of one lit cave wall in
  `runs/111-cave-glow-and-lamps-live/`, taken in the live game's
  conditions.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

New glow kinds, the glow's look, light strength.

## Log

### 2026-10-29: live reproduction blocked; no rendering cause established

- Read-only owner settings: `game/fallout2.cfg` has 2310x1298, scale 2,
  zoom 1.28, glow on, `glow_strength=1.0`, lights on. The test process used
  the owner's main-cache scene at
  `E:\Projects\fallout2-ce-3d\game\mod-cache\scene` and the owner executable
  (SHA-256 `ae939665d67e718f69c086e6c842348767a0dd9ca4f95d38a71a30ea237cfe72`);
  no owner-game process was stopped or restarted.
- Ran the same non-still capture twice, with `-Map klatoxcv -Elevation 1
  -ViewTile 22924 -ScreenWidth 2310 -ScreenHeight 1298 -Scale 2
  -SceneCache E:\Projects\fallout2-ce-3d\game\mod-cache\scene
  -CaptureConsole -Glow -GlowStrength 1 -TimeoutSeconds 90`. Both attempts
  ended `No snapshot within 90 s.` No JSON or PNG was produced. Both logs say
  `devtools: startup target klatoxcv e1 accepted=1 blocking=0`; stderr reports
  OpenGL 4.1.0, NVIDIA 617.14 / GeForce RTX 5070. This does not establish why
  glow or lights look different in the owner's live game.
- Validation possible without live captures: `ctest --test-dir build
  --output-on-failure` passed 15/15. `tools/regression.ps1 -Spot klatoxcv-e1`
  passed with 0 changed pixels. Its sidecar reports resolution 1920x1080,
  logical_resolution 960x540, scale 2, `ce_commit=1a5766a5`, executable hash
  above, and `glow.enabled=false`; this is the still pixel baseline, not the
  requested live glow/light before/after.
- Known unrelated main-baseline regression differences remain as previously
  recorded in task 100: `klagraz-e0` 183 pixels and `klaratcv-e2` 6299 pixels.
- No source or game files changed. Cause and live before/after evidence remain
  outstanding; status is `blocked` after two same-method attempts.
