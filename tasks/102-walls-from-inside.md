---
id: 102
title: Walls seen from inside are whole and dark, with no holes and no lit window strips
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

From inside a building in flight, its walls are whole, with no gaps to the
outside, and their inner face does not show the outside picture's windows
lit up.

## Context

The owner, 2026-10-27, live with the 095 build, shots in `runs/inbox/`:

- `2026-10-27-wall-holes-still.jpg`: inside a house at eye level the walls
  between the doors have gaps; the owner: the task about holes in walls
  changed nothing here. 071 ("Walls: no holes, a back side everywhere") is
  done, so this is what it left.
- `2026-10-27-bathhouse-inside-light-strips.jpg`: inside the bathhouse,
  roof off, the far wall's inner face shows vertical strips of bright specks
  where the outside picture has windows; the owner reads them as stray strips
  of light.

## Acceptance

- before | after of both views and two more interiors of Klamath at eye
  level, in `runs/102-walls-from-inside/`, with what the inner face of a wall
  shows now and why.
- The 090 seam totals do not rise; the original view regression stays pixel
  for pixel; ctest passes.

## Out of scope

Roofs (101), corner pieces (096), the table's junk (100).

## Log

### 2026-10-28 gpt
Investigation before implementation: exported only `kladwtwn/0` to
`build/scene-cache`. In the bathhouse wall band near `u=22.7..27.4, v=48.0898`,
rows 553 and 557 sample atlas sprites 1920 and 1924 on their backs
(`wall_back`, `back=-1`): 13.8% and 3.4% of those sampled texels have alpha
below 128. Row 556 at `u=26.7266..26.3932` uses sprite 1923, whose sampled
window pattern is fully opaque. This makes transparent outside texture and
opaque window detail two distinct causes to check for the bathhouse's inner
light strips; the row positions lie in the bathhouse shell, but the supplied
JPEG has no camera/tile metadata to prove which one is in view.

The wall-holes JPEG shows a different interior: dark wall spans between door
panels, with outside-colored gaps at their upper edges. A plausible map source
is row 308 (sprite 1839, `u=50.3568..49.6901, v=40.0352`), whose back sample
contains 11.6% transparent texels, adjacent to plain wall row 305 (sprite
1859, 0.7% transparent). The JPEG also has no camera/tile metadata, so these
are candidates only, not a verified match. The sampled texel data and wall
row coordinates are recorded here before implementation; next, validate the
two causes against runtime views and implement one general rule for inner
wall faces.
