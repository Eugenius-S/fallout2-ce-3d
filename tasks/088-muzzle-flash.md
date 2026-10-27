---
id: 088
title: A shot flashes at the gun's muzzle in the native animation's rhythm
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: [086]
reviewed: no
---

## Goal

When someone fires a gun, a flash of light blinks in front of them at the
muzzle. A burst follows the flashes painted in its animation. Each flash lands on the
frames where the shooter's own sprite brightens at the shot, so the light and
the picture blink together.

## Context

- The owner, 2026-10-26: the next stage of the glow work after 086 (gpt's
  self-lit weapon effects, local colored light and bloom). The flash uses
  the same three layers: a bright core, a short local light on what stands
  near, and the bloom, under the one global glow strength.
- Timing and place come from the art, not from a hand-made table: the
  firing animations (single shot and burst) already paint the muzzle flash
  as bright pixels near the barrel on some frames. Find those frames and the
  bright cluster's position per frame and rotation; the flash blinks on
  those frames at that spot. A burst's repeated bright frames give its
  repeated flashes.
- Energy and other non-bullet weapons: laser, plasma, pulse, electric
  (shock), the flamethrower, the rocket launcher, and any other kind the
  weapons' projectile art (`art/misc`) shows; list them from the art, not
  from memory. The bolt, flame or rocket leaving the barrel is already
  self-lit and casts its light by 086, so the muzzle glows by itself. If that reads too weak at the muzzle, add a smaller
  flash there, with the bolt's colour; show both, with and without it, and
  let the owner pick.
- The flash is visual only: no change to combat, timing, sound or the
  game's own frames.

## Acceptance

- Night sheets in `runs/088-muzzle-flash/`, laid out in rows: one row per
  weapon and rotation, the firing frames left to right in order, each frame
  labelled with its number; under each row the same frames without the
  flash. The flash sits on the same frames the sprite brightens.
- Rows for: a pistol, a rifle, a burst weapon (every painted flash of the burst
  visible), and every energy and non-bullet kind found (laser, plasma,
  pulse, electric, flamethrower, rocket, ...: their own glow, and with the
  smaller flash added); each at least at two rotations.
- Glow off: no flash; the original view regression stays pixel for pixel.
- JPEG sheets at most 1 MB each.

## Out of scope

New art, weapon balance, projectile trails, per-weapon menu controls.

## Log

### 2026-10-27 gpt: implementation and comparison in progress

The owner chose the painted flash rhythm: the SMG spends six rounds but its
three painted flashes get three light pulses. Native animation and combat timing
stay unchanged. Pixel masks and light sources follow the firing frames and
rotations, controlled by the existing global glow switch. A no-save fixture
captures frames with and without the new flash; the energy comparison places
the native projectile at the painted barrel and is not a combat replay. The
default energy mode uses the projectile's existing 086 glow; an extra smaller
flash is available only in the fixture for visual comparison. Weapons without
a separate projectile retain their painted muzzle or flame glow.

Build and 17 CTests, 64 exporter tests and 12 regression unit tests passed.
Bullet frame captures exist in build/muzzle088/bullets and the glow-off SMG
control in build/muzzle088/glow-off-smg. A laser-pistol pilot exists in
build/muzzle088/energy/laser-pistol-r1. The remaining energy captures run via
TEMP/088-capture-energy.ps1; save comparison sheets under runs/088-muzzle-flash.
Check the actual frames and off/on pairs, then run the full fly tour and normal
view regression against baseline main 261eaa2e. Task remains in progress.

### 2026-10-26 gpt: claimed after completing 086

Owner explicitly requested 088 after 086 merged as PR 122. Claimed on
task/088-muzzle-flash from main 261eaa2e. Worktree:
E:/Projects/fallout2-ce-3d-gpt-065 (reused, main checkout never switched).
No implementation changes yet. This chat is exceptionally long; the repository
AGENTS rule requests a fresh chat per task to avoid repeated context cost.

Next bounded step: inspect native firing animations/prototypes with the local
read-only f2lib exporter APIs, inventory actual weapon/projectile kinds and
identify muzzle-flash pixels/frames for two rotations. Do not invent timing
tables or change combat. Then add a no-save shooter/frame fixture and reuse
086's source/local-light/bloom pipeline behind the one common glow setting.
The original view and glow-off regression must remain unchanged. Energy and
other non-bullet kinds require own-glow vs extra-small-flash comparisons.

Useful existing modules: ce/src/scene/effect_emission.h, scene_lights.cc,
scene_objects.cc (shownObjectArt aligns source extraction with the camera),
ce/src/devtools/weapon_effect_fixture.{cc,h}, tools/dev-snapshot.ps1.
Use the isolated background instance and the global test mutex; never restart
or install into the owner's game without a separate request. Python is py -3.
Build helper on this host: TEMP/fallout065-build.ps1, Release. Klamath and Den
caches in this worktree are real fresh folders, not the old borrowed junctions.
Run the full fly tour against a built main checkout for renderer changes;
E:/Projects/fallout2-ce-3d-gpt-086-base currently holds main 317e3fef and must be
advanced/rebuilt before using it as a 261eaa2e baseline. Windows PowerShell
subprocesses may need process-only PSModulePath pointing at the Windows
PowerShell v1.0/Modules folder. No system environment changes are needed.
