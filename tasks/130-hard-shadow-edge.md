---
id: 130
title: No long straight hard shadow edge across the street
milestone: M1
status: todo
owner: none
suggested: glm
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The sun shadow on the ground never ends in a long straight hard line that
no object casts. Every shadow edge belongs to a wall, roof or thing and is
as soft as the other sun shadows.

## Context

- The owner, 2026-11-01, live on main `e434c327`, Klamath downtown in the
  game camera: `runs/inbox/2026-11-01-hard-shadow-edge.webp`. A dark area
  covers the upper left of the street and ends in a long straight diagonal
  edge running from the lamp post toward the lower right, through open
  ground. The owner took it for cloud shadows; cloud shadows (023, PR #74)
  are not on main, so it is something else.
- Suspects: the edge of the shadow map's coverage or a cascade boundary, a
  roof or wall plane casting from outside the view, the 109 receiver mask.
  Find which, then fix it by a rule.
- Check other hours and maps: if the same edge shows elsewhere, list them.

## Acceptance

- before | after of the owner's spot at morning, noon and evening, in
  `runs/130-hard-shadow-edge/`, and the cause written in the Log.
- ctest passes; the original view regression changes only where the edge
  was; the fly tour changes are explained.

## Out of scope

Cloud shadows (023), shadow softness tuning beyond matching the other sun
shadows.
