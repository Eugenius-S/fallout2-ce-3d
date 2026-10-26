---
id: 067
title: A person standing before a cabinet draws in front of it
milestone: M1
status: doing
owner: claude
suggested: glm
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A character standing in front of a tall thing (a cabinet, a locker) draws in
front of it, as in the game; the 3D view must not put the thing over the
person.

## Context

- The owner's live review of 2026-10-24, shots 25..35 in
  `runs/046-characters-in-shadow/review-2026-10-24/`: a cabinet draws in
  front of a person standing before it.
- Cards sort by `spriteRowPlane` (live_objects.cc); 051 just moved hanging
  smoke one row south. Find whether the cabinet's card or its fitted solid
  takes the wrong plane, then fix the rule, not the one cabinet.

## Acceptance

- Before and after snapshots of the case under
  `runs/067-card-order-cabinet/`, in the game's view and a turned one.
- The regression stays where it is.

## Out of scope

Shapes of the things themselves (068).

## Log
