---
id: 116
title: The stove and the lockers stand as solids
milestone: M1
status: todo
owner: none
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: [115]
reviewed: no
---

## Goal

The stoves stand as drums with their lid on top, and the lockers as boxes,
in the 3D view; neither stays a flat picture.

## Context

- The owner, 2026-10-28, live on main `ff5f4ed0`:
  - `runs/inbox/2026-10-28-stove-flat.png`: the stove is a flat card.
    100 removed its drum override because the projected lid wrapped round
    the cylinder and read as a torn disc. The answer is a drum whose top
    face takes the lid, not a card.
  - `runs/inbox/2026-10-28-locker-flat.png`: lockers are tall boxes in the
    art; the owner: easy to make 3D.
- By rule, not by name lists: what in the art or the prototype tells a
  drum with a lid from a box (shapes/fits in `tools/scene_export`).

## Acceptance

- before | after of the bathhouse stove, the other Klamath stoves and the
  lockers, game angle and flight, in `runs/116-stove-and-lockers-solid/`.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

Holes in solids (115), other furniture kinds, the landmark objects.

## Log
