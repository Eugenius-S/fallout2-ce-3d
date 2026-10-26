---
id: 073
title: Glow strength ladder: each glowing thing at five strengths for the owner to pick
milestone: M1
status: done
owner: gpt
suggested: glm
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

The owner wants every glow to read clearly ("прям ощутимыми"). They pick the
strength from test shots: each kind of glowing thing shot at five strengths,
1 the faintest to 5 the strongest, side by side.

## Context

- The glow is 022 (emissive buffer, bright pass) and 050 (the wide halo of
  the marked cycling colours); `[3d] glow_strength` scales it, clamped to
  0..3, default 1 (`ce/src/settings.cc`). The ladder may need more than 3 at
  its top: widen the clamp if level 5 calls for it.
- A background run forces the glow off unless `-Glow`; it has no way to set
  the strength yet. Add one (`--dev-glow-strength=X`, a `-GlowStrength X`
  switch in `tools/dev-snapshot.ps1`), as `-Hour` was added for the sun.
- The kinds, at least: a burning barrel and an open fire (Klamath), a
  computer monitor and the vault's screens (`vault15`, live cycling colours
  per 050), the toxic slime (klatoxcv), an energy weapon's bolt if a fixture
  can show one. Night views where the map has them: a glow reads in the dark.
- If the halo and the near glow should scale apart (the owner may want a
  wider halo, not a brighter core), shoot that as a second ladder for one
  kind and say so in the Log.

## Acceptance

- `runs/073-glow-ladder/<kind>.jpg` for each kind: one sheet with the same
  close view at strengths 1 to 5 in a row, each labelled with its number and
  the `glow_strength` value it maps to; plus the same view with the glow off.
- The owner's pick goes into the Log and becomes the default in
  `settings.h` and `settings.cc` (user-check: yes).
- Owner's final scope: one global strength, default 2 for fire; compensate
  brighter white/cyan computer emission internally. No per-kind controls.
- Correct cycling colors on the wall monitor and slime; frame the entire
  selected pool, with space around it to see the halo.

## Out of scope

New glowing things (what is not marked or bright today).

## Capture pipeline

INPUT: marked emissive RGB, near bright pass, one global glow scalar (default 2).
TRANSFORM: derive peak and Rec.709 luminance; select cool pixels by B/peak.
TRANSFORM: cool compensation clamps 0.2126*peak/luminance to 0.5..1; mix by smoothstep(0.25,0.65,B/peak).
MEANING: white/cyan emission is reduced; red fire and green slime keep gain 1.
OUTPUT: both existing halo scales share the compensated RGB and one menu scalar.
DIRECT CHECK: same-palette before/after crops, fixed rest-phase ladders, glow-off regression.
The fixture uses cycle.cc's colors, not unmapped color.pal/_cmap entries.
JPEG sheets follow AGENTS.md; raw PNG captures stay untracked under build/.

## Log

### 2026-10-26 gpt
Done: developer `-GlowStrength X` / `--dev-glow-strength=X`, range 0..8,
plus `-GlowLadder`: one frozen process captures OFF then 0.5/1/2/4/8.
The menu slider accepts 8; default remains 1 pending the owner's choice.
Fixed comparison setup: restore the authored cycling palette explicitly,
and omit the HUD in every ladder capture (otherwise it appeared in later
frames only). Ordinary captures/gameplay keep their old overlay behavior.
Evidence: five individual JPEG sheets and two combined sheets under
`runs/073-glow-ladder/`; `tools/glow_ladder_sheets.py` validates capture
metadata and produces crop statistics plus raw PNG hashes.
Observed: barrel/working comp9 respond, campfire weakly. The sampled pool
and monitr7 wall screen do not form a visible halo even at 8 (6/4 crop
pixels change by >8/255). Their separate investigation is in inbox; no new
emission marks were added. No energy-bolt fixture is currently available.
Check: Release, 15 CTests, 64 exporter and 12 regression unit tests pass.
All 16 ordinary views match independent current-base controls from 066
exactly, 0 changed pixels. Old committed references were not changed (058).
Next: OWNER PICK REQUIRED: level 1=0.5, 2=1, 3=2, 4=4, 5=8. Apply their
choice as the default, update checks, complete the PR. Task remains doing;
no default was guessed. The owner's game/config were not restarted/installed.

### 2026-10-26 owner selection
Approved reference looks (actual glow_strength values, not level numbers):
- Barrel: level 2, strength 1.
- Working computer: level 3, strength 2.
- Campfire: level 4, strength 4.
Owner also confirmed that the wall screen and slime have no visible glow.
These approvals select different strengths by object kind. The implementation
currently has one global scalar, so assigning one new default cannot reproduce
all three approved looks. Do not substitute a single compromise value.
Next: resolve the per-kind implementation scope before changing defaults;
the dark-screen/pool issue remains recorded in inbox. Keep 073 doing.

### 2026-10-26 owner scope update
The owner chose a common fire strength of 2 for barrel and campfire, with
luminance compensation for white/cyan computer glow. Keep one global menu
slider. Produce corrected monitor/slime comparisons and frame the entire
selected slime pool with space around it. This supersedes the earlier
per-kind proposal and requires no per-kind setting or new emission marks.

### 2026-10-26 gpt: completed final owner scope
Done: one global menu scalar, default 2 in settings.h and the config writer.
White/cyan computer emission is luminance-compensated with gain 0.5..1;
warm red and green source colors retain gain 1. No per-kind controls.
Corrected the test's real cause: _cmap has unmapped zeros for cycling colors.
The fixture now uses cycle.cc's rest-phase palette. Both monitr7 and slime
produce halos; the earlier no-response measurements are superseded, and the
inbox entry is marked resolved. Reframed the entire slime pool at view 22924,
zoom 1, with visible space around it; screen view now includes its room.
Evidence: regenerated all five ladders; default-fires, compensation,
screens-detail and slime-detail JPEGs under runs/073-glow-ladder/.
Check: measured same-palette compensation at strength 2: added RGB sum ratios
computer 0.4957, barrel 0.9302, campfire 0.9968, slime 1.0000. OFF crop pixels
are identical in all four controls. The barrel ROI includes some cool spill.
Release game/mapper, all 15 CTests, 64 exporter and 12 regression unit tests
pass. All 16 glow-off world views match 066 current-base controls exactly;
committed old references were not retaken (058).
Next: owner checks the final visuals after merge. No remaining owner pick;
the selected default and single-slider scope are implemented. No owner game
restart, installation or config rewrite was performed.
