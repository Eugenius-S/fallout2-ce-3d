---
id: 123
title: Burnt rebar stands: second-floor wall ruins continue the wall
milestone: M1
status: todo
owner: glm
suggested: owner
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The owner, 2026-10-31: on some buildings' second floors the burnt rebar
(the scorched reinforcement sticking out of the ruin) renders lying down;
it must stand as a continuation of its wall, not lie on the floor or the
roof.

## Context

- The rebar art rides the wall ruins (second-floor roof edge pieces); today
  it lands as a card that lies along the surface instead of standing on the
  wall's plane.
- The fix is exporter-side: such pieces belong to their wall's upright plane
  (a panel or sheet on the wall), like the wall scenery rule of 100/116
  (wall scenery keeps wall-sheet sorting).

## Acceptance

- Before | after pairs of the affected buildings: the rebar stands on the
  wall line, no piece lies flat across a roof or floor.
- The game's views stay pixel for pixel on the regression spots; ctest
  passes.

## Out of scope

New art, engine work, other ruin fixes.

## Log

### 2026-10-31 glm: opened from the owner's order

Queued last. Recon first: find the rebar arts and which sidecar path they
take today (panels, sheets, or boards).
