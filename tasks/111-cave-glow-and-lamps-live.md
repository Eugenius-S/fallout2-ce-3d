---
id: 111
title: The slime pools glow and the lamps light the walls in the live caves
milestone: M1
status: todo
owner: none
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

### 2026-11-01 glm: the glow and the lights verify in the owner's conditions; the live capture tool hangs

Done: the reproduction sweep, no code change.

- The still fixture in the owner's own mod-cache (klatoxcv e1, view 22924,
  the 073 slime spot) shows the slime pools glowing and the pool light
  spilling on the floor and the barrels: runs/111-cave-glow-and-lamps-live/
  (cave-still-glow.jpg). The same frame with the sun shadows forced on
  (-Shadows) is identical: the cave is indoor and the shadow pass honours
  it, so the owner's shadow settings are not the cause.
- The settings check: glow defaults true, lights true; the owner's
  glow_strength 1.0 only weakens the halo (the 073 ladder shows a visible
  glow at 0.5). His other custom keys (sun_swing, shadow_density,
  shadow_pixel_size, sharp_text) do not touch the cave lighting.
- The palette cycle cannot remove the glow: the emission classification is
  by sprite index (the marked texels 252-254), the cycle only recolours
  the entries; the live object textures rebuild from the live palette and
  keep their indices.
- The lights are the game's own per-tile light state read live each frame;
  nothing in the still fixture freezes them.
- The live (non-still) capture is blocked by a tool defect: a plain run,
  -NoPreview, -WalkProbe and -ScrollStep all time out after "startup
  target accepted" with no snapshot. In the live path the snapshot trigger
  (devtools.cc scenePresented, the gSnapshotOncePending branch) is never
  reached: with the walk preview the flow stays in the startup preview
  block, and the scroll/walk probes' stage guards return before it. The
  still path has its own trigger inside the startup block and works. This
  is gpt's PR #156 blocker, now localized. Recorded as a tool task, not
  part of this fix.

owner: were the caves seen in the 3D view? The 2D view (Ctrl+F10 toggles
the 3D off) has no glow and no point lights by design, and the runtime
toggle survives map changes in a session. If it was the 3D view: the
save from that moment would settle it in one still run
(dev-snapshot.ps1 -Slot N -Still -CyclePalette -SceneCache
game/mod-cache/scene).

Next: the owner's answer or save; the live-capture tool defect as its
own task.
Check: the cave-still-glow.jpg pair in runs/111-cave-glow-and-lamps-live/.
