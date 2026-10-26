---
id: 077
title: The camera turns up to 45 degrees; more only by the player's choice
milestone: M1
status: done
owner: gpt
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

The released game turns its camera at most 45 degrees either way from the
game's own view; a default-off menu option marked "not recommended" lifts
the limit.

## Context

- The owner, 2026-10-25: the camera ships limited; past 45 degrees is at
  the player's risk. Seen from further round, the 2D-to-3D conversion shows
  backs and corners it cannot know, so the limit keeps the default look
  clean. Building corners and walls must still meet right within the
  limit (076).
- Today: `camera_rotation` in `[3d]` (`ce/src/settings.cc`, the options
  panel `ce/src/scene/options_panel.cc`), 90-degree steps from 038, off by
  default; the free dev camera (031) stays unlimited.
- Every new setting has a key, a default and a menu line (the roadmap).

## Acceptance

- With default settings the turn stops at 45 degrees either way; the new
  option lets it go round; both in the menu.
- Shots at -45, 0 and +45 degrees on kladwtwn in
  `runs/077-camera-45/`; regression 16/16.

## Out of scope

What the turned views look like (075, 076).

## Implementation

INPUT: [ and ] request signed steps; two Camera menu toggles control rotation.
TRANSFORM: add 45 degrees per step, clamp the target to -45..+45 unless unlimited.
MEANING: held input cannot cross the safe endpoints; opposite input returns to zero.
OUTPUT: the existing animated orbit and look-only mode; zero restores the game view.
DIRECT CHECK: endpoint/reversal/full-turn tests, -45/0/+45 captures, 16 unchanged views.
Dev snapshot angles and the free fly camera bypass the player limit.

## Log

### 2026-10-26 gpt
Done: player [ and ] steps are 45 degrees, clamped to -45..+45 by default.
The separate default-off `camera_rotation_unlimited` Camera menu toggle
permits a full turn and warns of missing backs/corners, in English and Russian.
Disabling rotation lands the camera; revoking unlimited wraps and clamps the
current angle. Dev flight and explicit snapshot angles remain unrestricted.
Legacy `-Turn N` still uses quarter turns; `-TurnDegrees D` adds exact angles.
Evidence: `runs/077-camera-45/` has -45/0/+45 views and the Russian menu.
Check: Release build, 15 CTests, 61 exporter and 12 regression unit tests pass.
All 16 world captures match current-base controls from 066 exactly (0 pixels).
Committed older references were not retaken; their known drift stays in 058.
Not checked: real keyboard input/animation in the owner's game; endpoint
math and actual menu controls were tested, angles rendered in isolated runs.
Next: owner's visual check; no game restart or installation performed.

UX copy update: the owner requested plain labels and "not recommended"
instead of "own risk". Removed "experimental" from Camera rotation and
rewrote both RU/EN tooltips around the actual action and visible consequence.
Rebuilt, passed settings_3d_tests, recaptured the Russian menu crop.
The final Russian label is "Full turn (not recommended)"; the longer
"Unlimited rotation" wording was clipped, so the snapshot verifies the
complete warning fits the label column.

### 2026-10-26 gpt: range label follow-up
Done: owner's exact Russian wording now states the -45 to +45 degree range
in the first menu label. Added degree-sign encoding and a CP1251-compatible
minus fallback; the native game encoding has no U+2212 glyph.
Check: Release build, settings_3d_tests (signs/degrees/no replacement marks),
real sharp-text preferences capture in `build/camera077/menu-range`.
Updated `runs/077-camera-45/options-russian.jpg`; the full label fits.
Next: no behavior change; existing 16-view regression remains applicable.
