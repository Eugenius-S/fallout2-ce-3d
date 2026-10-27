---
id: 101
title: Roof edges are clean, the roof fringe stands up like a wall, no wall tops through roofs
milestone: M1
status: todo
owner: none
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Seen from the air, a roof meets its walls cleanly: no dark steps along its
end, no wall tops poking through it, and the ragged fringe at a roof's edge
rises as a continuation of the walls instead of lying flat on the roof.

## Context

The owner, 2026-10-27, live with the 095 build, shots in `runs/inbox/`:

- `2026-10-27-roof-end-flaw.jpg` and `2026-10-27-roof-end-three-flaws.jpg`:
  along the end of the big thatched roof the dark wall top steps in and out
  under the roof's edge (one flaw on the first, three on the second).
- `2026-10-27-roof-horns.jpg`: dark wall-top strips stand up through the big
  brick building's roof (a broken dashed line, a black triangle at its right
  edge) and through the roof left of the bathhouse. Fewer than before 081,
  still there.
- `2026-10-27-roof-fringe-lies-flat.jpg`: the house with the ragged dark
  fringe (by the cow pen): in the original the fringe hangs down the wall
  top; in 3D it lies flat on the roof. It should rise from the walls like
  the walls do, as their continuation.
- Earlier roof work: 081 (roofs and overlaps), 071 (walls whole).

## Acceptance

- before | after from the air of the three roofs above and their original
  view at the same scroll, in `runs/101-roof-edges-and-fringe/`; the other
  Klamath roofs looked over the same way.
- The 090 seam totals do not rise; the original view regression stays pixel
  for pixel; ctest passes.

## Out of scope

Shadows on roofs, corner pieces (096), the walls' inside (102).

## Log
