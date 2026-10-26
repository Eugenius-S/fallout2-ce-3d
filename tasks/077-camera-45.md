---
id: 077
title: The camera turns up to 45 degrees; more only by the player's choice
milestone: M1
status: doing
owner: gpt
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The released game turns its camera at most 45 degrees either way from the
game's own view; a menu option off by default, marked as at the player's
own risk, lifts the limit.

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
