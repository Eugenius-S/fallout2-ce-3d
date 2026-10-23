---
id: 023
title: Cloud shadows
milestone: M1
status: doing
owner: glm
suggested: glm (045's shadows landed in PR #73, the branch stacks on it)
claude-only: no
user-check: yes
depends-on: [045 (PR #73, stacked)]
reviewed: no
---

## Goal

Soft cloud shadows drift across the ground in daylight.

## Context

There are no visible clouds (no first-person view); only their shadows. They
dim the sun's light only, not lamps; the sun term comes from task 021.

## Acceptance

- A tiling cloud pattern moves slowly with a wind direction across floor,
  walls and sprites (sprites by their anchor, like building shadows).
- No cloud shadows at night or indoors; their strength follows the sun.
- `[3d] cloud_shadows` (default on) with a menu label.
- A short sequence of snapshots in the PR shows the motion.

## Out of scope

Weather.

## Log
