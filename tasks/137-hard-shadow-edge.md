---
id: 137
title: No long straight hard shadow edge across the street
milestone: M1
status: done
owner: claude
suggested: claude
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
- The owner, 2026-11-01, on main `fc37980e` with `cloud_shadows=0`: the
  Klamath downtown street still shows big dark shapeless patches
  (`runs/inbox/2026-11-01-sun-shadow-blotches.png`). With the clouds off
  these are sun shadows (`shadows=1`, `sun_swing=44`,
  `shadow_pixel_size=3`, `shadow_density=0.85`). Find what casts them; the
  same cause as the straight edge is likely.
- Check other hours and maps: if the same edge shows elsewhere, list them.

## Acceptance

- before | after of the owner's spot at morning, noon and evening, in
  `runs/137-hard-shadow-edge/`, and the cause written in the Log.
- ctest passes; the original view regression changes only where the edge
  was; the fly tour changes are explained.

## Out of scope

Cloud shadows (023), shadow softness tuning beyond matching the other sun
shadows.

## Log

### 2026-11-01 gpt
Check: Compared Klamath downtown at 08:00, view tile 24678, with cloud
shadows alone and with shadows off. The captures use the same map, camera,
resolution and executable. See `runs/138-desert-clouds/137-cloud-edge-check.jpg`
and its JSON sidecars.

Result: The reported long, hard edge did not reproduce. Cloud-only shading
appears as irregular patches. The shader applies a broad `smoothstep` to
noisy coverage, so its current cloud path has no straight clipping boundary.

Limit: The cloud-only run has sun shadows off, and the comparison does not
recreate the owner's exact view. This is preliminary evidence against a cloud
edge, not a confirmed root cause. The shadow-map, caster and receiver-mask
suspects remain open; task 137 stays todo.

### 2026-11-01 follow-up
The newer clipboard crop is 193x321 and has no game metadata. Its strongest
image match in the matched KLADWTWN/tile 24678 captures is at (333, 601),
normalized correlation about 0.932. At 08:25, swing 44 and cloud phase 505,
that region is unchanged when both shadow modes are off; clouds change it,
while sun shadows alone do not. The edge persists with both modes off, so
neither shadow pass is shown to create it in this matched region. See
`runs/138-desert-clouds/ab-0825-s44-p505-shadow-modes.jpg` and sidecars.

Limit: the image match does not prove identical owner runtime state, and the
material or geometry source remains unknown. Task 137 stays unresolved; this
is preliminary attribution only.

- 2026-11-01 Moved to claude with the owner's sun shadow blotches find (inbox).

### 2026-11-01 claude
Cause: no shadow defect. The straight edge and the shapeless patches are the
Klamath asphalt itself: the road art has near-black and dark grey zones with
fairly sharp borders, and they read the same in the game's 2D picture.

Check: the owner's spot (lamp post at the curb by the Buckner building,
`-ViewTile 22491`) with the owner's shadow values (`shadow_pixel_size=3`,
`shadow_density=0.85`, `sun_swing=44`), in the background test run:
- `runs/137-hard-shadow-edge/blotches-are-art.jpg`: the owner's shot, the
  2D picture, 3D with every shadow off and 3D with sun shadows at 08:00. The
  patches are in all four.
- `runs/137-hard-shadow-edge/sun-shadow-only-08-12-17.jpg`: shadows off
  minus sun shadows at 08:00, 12:00 and 17:00, amplified. The sun shadows
  darken only the building walls, the sidewalk at their foot and the fence;
  nothing on the road.

Next: if the owner still sees a patch that turns off with `shadows=0`, a new
task with that view.
