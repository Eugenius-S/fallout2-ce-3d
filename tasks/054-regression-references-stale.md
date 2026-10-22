---
id: 054
title: Regression references no longer match the unchanged renderer
milestone: M1
status: todo
owner: none
suggested: claude
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Make the regression suite green again on the dev machine without hiding a
real rendering change: find why 13 of 16 spots differ from the references
that task 034's merge retook, then retake or fix.

## Context

- Found on 2026-10-22 while taking task 050's glow snapshots.
- With task 050's renderer changes stashed (the reverted build), a full
  regression run failed the same way: entrance 501, corrals 361, residents
  380, central-doors 133, north-rooms 396, map-edge 1085, klagraz-e0 74,
  klamall-e0 3235, klaratcv-e1 821, klaratcv-e2 3847, klatoxcv-e0 836,
  klatoxcv-e1 22777, klatrap-e0 2130 changed pixels; klacanyn-e0,
  klaratcv-e0 and klatoxcv-e2 pass with 0.
- The differences sit on sharp edges (a subpixel shift) and on klatoxcv's
  drums, which render visibly wider; floors, walls and the slime are
  pixel-identical, and two runs the same day are byte-identical to each
  other, so the state is deterministic today.
- The scene caches are byte-identical to the game's mod-cache copies, the
  game data files have no new timestamps, and the exe is main's code, so
  the cause is either the environment (the GPU driver changed?) or a merge
  that slipped past the suite. The references were last retaken by 034
  (8543064e).
- Klatoxcv elevation 1 and 2 snapshot runs also hung twice that day with no
  output while other maps captured; later the same spots captured fine with
  `-Still`. Suspected: a hidden modal box (debug.log has a "Program already
  running" line) when the owner's game was open. dev-snapshot could detect
  a modal box in a background run and fail fast instead of waiting out the
  timeout.

## Acceptance

- The cause is named in this task's `Log` (environment or code).
- `tools/regression.ps1` passes 16/16 again, references retaken only if the
  change is confirmed intentional.

## Log
