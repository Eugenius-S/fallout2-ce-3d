---
id: 083
title: Corner decor in 3D: the corner picture folded into two sprites
milestone: M1
status: todo
owner: none
suggested: claude
priority: high
claude-only: no
user-check: yes
depends-on: [076]
reviewed: no
---

## Goal

Houses with balusters, gargoyles or pilasters on a corner keep that decor in
the fly view, sitting on the corner, while the walls still meet cleanly.

## Context

- Agreed with the owner on 2026-10-25 (`runs/inbox/2026-10-25-corner-decor-plan.png`).
  076 leaves every corner piece out in flight and fills each face to the
  edge in the wall's plain material: right for plain corners, but a decor
  corner loses its decor.
- The plan:
  - the wall up to the decor repeats its last 8 to 16 columns before the
    corner piece (not one stretched pixel column: that is the striped
    strips 076 removed);
  - the corner picture is folded at the column where the corner's edge
    projects on the game's screen: one half lies in one wall's outer plane,
    the other half in the other's, a hair outside the fill, so from the
    game's camera it is the original art pixel for pixel; an outward wedge
    (the owner's sketch) only as a tunable offset, chosen from shots;
  - a corner piece counts as decor when its picture differs from the plain
    wall (brightness and pattern away from `_typical_columns`' median) or
    reaches beyond the wall's outline (above the cap); plain corners keep
    076's fill. Found by rule, no list by hand.
- Code: `tools/scene_export/wall_sidecar.py` (`_corner_fits`, `pieces`,
  `_corner_fill`); check with `corner_rays.py` and fly shots.

## Acceptance

- Decor corners on kladwtwn and klamall before | after from turned views
  within 45 degrees in `runs/083-corner-decor-fold/`.
- No corner gains holes (`corner_rays.py`), the game's own view 0 px.

## Out of scope

Roofs and walls through walls (081).

## Log
