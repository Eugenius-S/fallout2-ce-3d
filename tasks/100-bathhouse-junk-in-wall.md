---
id: 100
title: The junk on the bathhouse table stands on the table, not in the wall
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

The things lying on a table in the 3D view stand on that table, as in the
original, and nothing of them shows through a wall from outside.

## Context

- The owner, 2026-10-27, live in the 3D view (093 leftover 4):
  - outside, a grey box sticks out of the bathhouse's long alley-side wall
    near its south corner (`runs/inbox/2026-10-27-shadow-leftover-4b-bathhouse.jpg`);
  - roof off, it is the junk of a table inside the bathhouse
    (`-4c-bathhouse-inside.jpg`);
  - from above in flight (`-4d-bathhouse-top.jpg`) the junk is not on the
    table at all: a box, a plate and cans sit against the alley wall, and
    two grey pipes lie outside it in the alley.
- 093 compared the painted crates outside that corner and found them equal
  to the original; that was a different spot. This is a scene object
  (items on a table) placed away from its table.
- The same class, same day: a round stove inside a house has its top apart
  from its body and sticks out through the wall behind it
  (`runs/inbox/2026-10-27-stove-top-apart.jpg` inside,
  `-stove-through-wall.jpg` from the street). Fix both by one rule.
- Earlier work on the same bathhouse table: 075 (things' shapes) and the
  inbox line "A table in the bathhouse sticks through its walls".

## Acceptance

- Find which objects these are (map, tile, prototype) and why the 3D view
  puts them off the table; fix the cause, not this one table.
- before | after of the three owner views (outside, roof off, from above)
  and the original view at the same scroll, in `runs/100-bathhouse-junk-in-wall/`;
  the other tables with things on them in Klamath checked the same way.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

Shadows, the corner pieces (096).

## Log
