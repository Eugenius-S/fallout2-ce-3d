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
`build/scene-cache`. The owner JPEGs have no camera/tile metadata, so their
exact camera tiles cannot be recovered. I matched their wall patterns and
nearby map objects to the source wall families, then reproduced both patterns
in the runtime snapshots below.

The door-wall view matches the `doorb06.frm` room and the neighboring
`nec` wall family: wall rows 225, 253, 264, and 292 are `nec107.frm`,
`nec108.frm`, `nec102.frm`, and `nec079.frm`; rows 253 and 260 have about
51% transparent texels in the sampled back strip (atlas sprites 1843 and
1801). The scene's door object at tile 15272 is `doorb06.frm`. The runtime
view at tile 15272 shows outside-colored pinholes in those dark inner wall
faces. These alpha holes explain the openings in the owner's first view.

The bathhouse view matches the two narrow windows on its far wall: rows 200
and 210 are `nec151.frm` and `nec150.frm` windows at tiles 14714 and 14722;
both sample back sprite 2216, whose dark texture has 7.3% transparent texels.
The neighboring wall panels are `nec096.frm`, `nec097.frm`, and `nec098.frm`.
The runtime view near tile 14900 shows the same light leaking through this
inner window pattern. It is the back texture's transparent pixels, not bright
opaque specks in the source, that reveal the outside.

Correction: the earlier rows 553/556/557 and row 308 were unrelated nearby
walls and are not the source pieces for these views. Do not use them as
implementation targets. Next: one general rule that makes wall-back texture
holes opaque and subdued for every wall, then retake the owner views and the
other Klamath interiors.
