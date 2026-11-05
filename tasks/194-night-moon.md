---
id: 194
title: The night looks like night, a weak cool moon light over the 3D view
milestone: M1
status: done
owner: gpt
suggested: any
priority: low
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
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

Added a cool, dim moon directional light that reuses the existing sun shadow map. It fades in from 18:00 to 19:30 and fades out from 04:30 to 06:00. The night sky keeps its stars. No extra shadow pass was added.

Live comparison: `runs/194-night-moon/comparison.jpg`, with the full noon, dusk and midnight frames and JSON sidecars. All three use KLADWTWN elevation 0, tile 24678, full `scene-gl`, the same camera and 1920x1080 output. Midnight is darker and cooler than noon while the scene remains readable. Evidence JPEGs are under 1 MB.

Performance: matched live runs used the same executable, map, tile, renderer, resolution and shadows. After excluding the first three startup windows and the final capture/exit window, seven frame-time windows averaged 16.84 ms at noon and 16.70 ms at midnight. Midnight is 0.14 ms faster.

Checks: Release build passed; CTest 20/20, scene-export Python tests 125/125, regression-tool Python tests 12/12. Full regression passed all 106 spots and elevations with 0 changed pixels.

The fly-tour captured 57/57 views before and after. Its comparison checkout was 27428dd7 instead of origin/main f6ffaa2b and contained unrelated PR #299 camera and scene changes, so that changed-pixel sheet is not treated as a valid main-baseline result.

PR #301 is ready for review after CI on the evidence commit.

