---
id: 194
title: The night looks like night, a weak cool moon light over the 3D view
milestone: M1
status: todo
owner: any
suggested: any
priority: low
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

At night the 3D view changes visibly: a dim, cool moon light takes the
sun's place, and the sky (142) shows its night mood with it.

## Context

- The owner, 2026-10-25 (from the inbox): at night nothing changes
  visually; there should be a moon of some kind, a dim cool directional
  light like the sun but weak.
- Stands alone: no open sky or light task. 142 (the desert sky, #204) and
  153 (the sky out of the glow, #219) are done; the sun lives in
  `ce/src/scene/sun.cc`, its settings in the [3d] options (`sun_swing`,
  `shadow_density`).
- Speed rule (AGENTS.md): no new shadow pass; the moon reuses the sun's.

## Acceptance

- Shots of one Klamath spot at noon, dusk and midnight: the midnight one
  is darker and cooler, with soft moon shadows, and still readable.
- The frame time at midnight is no worse than at noon (127's log).

## Out of scope

Lamps and glows (111), clouds (143).

## Log
