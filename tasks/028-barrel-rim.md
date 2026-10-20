---
id: 028
title: The rim of a burning barrel breaks off in 3D
milestone: M1
status: doing
owner: claude
suggested: claude
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A burning barrel in 3D shows its whole rim, as in 2D.

## Context

- The owner's screenshot (`runs/028-barrel-rim/owner-report.png`, Klamath,
  3D, build of PR #20): the top edge of a burning barrel, circled in red,
  stands apart from the barrel's body, as if broken off.
- The barrel is scenery made solid by the exporter (`tools/scene_export/solids.py`,
  baked into `walls.bin` by `wall_sidecar.py`); its fire is a separate
  animated sprite. Find first whether the rim belongs to the baked solid's top
  or to the fire sprite's card, and compare with 2D at the same spot.

## Acceptance

- The barrel's rim is whole in 3D at the owner's spot and on the other
  burning barrels of Klamath; snapshots 2D | 3D in the PR.
- Regression snapshots unchanged elsewhere.

## Out of scope

Glow of the fire (022).

## Log
