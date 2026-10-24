---
id: 051
title: Smoke over a burning barrel draws in front of it, as in the game
milestone: M1
status: doing
owner: glm
suggested: gpt
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The smoke and flames of a burning barrel are drawn over the barrel as the
classic 2D game draws them, not behind it.

## Context

- The owner's find, 2026-10-21: the smoke goes behind the barrel
  (`runs/051-smoke-order/owner-smoke-behind-barrel.png`, Klamath).
- First check the classic 2D at the same spot (`dev-snapshot.ps1 -Classic`):
  if the game also draws the smoke behind, there is nothing to fix, say so in
  the Log and close.
- Cards take the depth of their hex row's plane (`spriteRowPlane`,
  `ce/src/scene_renderer/sprite_card.h`); a smoke object on the hex behind the
  barrel lands behind it. The barrel's rim and flames were handled in 028
  (`tasks/028-barrel-rim.md`).

## Acceptance

- The smoke over the barrel as in the 2D view, before | after in
  `runs/051-smoke-order/`.
- The regression passes or its changed views show only this fix.

## Log
