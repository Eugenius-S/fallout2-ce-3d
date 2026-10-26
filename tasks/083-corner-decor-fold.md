---
id: 083
title: Corner decor in 3D: the corner picture folded into two sprites
milestone: M1
status: done
owner: claude
suggested: claude
priority: high
claude-only: no
user-check: yes
depends-on: [076]
reviewed: yes
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
### 2026-10-25 claude
Done: decor corners keep their decor in flight, folded onto the corner.
- Decor by rule (`_decor_pieces`): a dull corner piece (mean channel
  spread at most 12) that is 8 or more darker or lighter than the plain
  rows near its end, or whose columns in 30% or more match no plain column
  within twice the plain wall's own spread (sliding 12 rows up or down).
  A colourful piece is brick or boards, the wall's own material: folded,
  the STOP corner's bricks came out skewed, so those keep 076's fill.
  kladwtwn: 36 of 87 pieces are decor, the same set as a look at each by
  hand (pilasters, knobs, gargoyle balls, stone corners); klamall 51 of 95
  (the fence posts, the dark mall corners, some corrugated corners).
- Fold (`_corner_decor`): each picture column lies on the front the game's
  camera sees there (from where each front ends at the corner, the nearer
  where both stand), DECOR_LIFT 0.004 out of the face, each point sampling
  the texel the game drew it on; the sideways corner (one front inside,
  one outside: the gargoyle at 19108) no longer hangs in the air. No wedge:
  the shots did not ask for one.
- Fill up to decor: both fronts repeat the last 16 columns of the nearest
  plain row at least 8 wide (`_corner_fill` repeat); plain corners keep
  076's typical-columns block.
- Check: `tools/scene_export/corner_rays.py` over the klamath group: no
  corner gains rays (kladwtwn 3 corners at 2 grazing rays as in 076;
  klamall 7 -> 6 corners, 29536 222 -> 178, 16250 closed; klaratcv 2 the
  same). The game's batches in walls.bin are identical on all 12 klamath
  scenes; game-view snapshots of kladwtwn 19108, 20690 and klamall 15450
  0 px. Exporter tests 55/55 (3 new: fold, rule, repeat).
- Before | after in `runs/083-corner-decor-fold/decor-corners-before-after.jpg`.
Next: the owner looks at the sheet; a corner the rule gets wrong becomes a
new task with its tile.
Check: `py -3 tools/scene_export/corner_rays.py <scene dir>`; fly shots
`tools/dev-snapshot.ps1 -Map kladwtwn -ViewTile 19108 -Still -Fly -FlyTurn 40 -FlyPitch 25 -FlyDistance 5`.
