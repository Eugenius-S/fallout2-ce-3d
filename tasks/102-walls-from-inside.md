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
