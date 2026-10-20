---
id: 035
title: Boards, shelves and junk piles stand as the donor stands them
milestone: M1
status: doing
owner: claude
suggested: claude
claude-only: no
user-check: yes
depends-on: [033]
reviewed: no
---

## Goal

From the air the things that are neither cards nor boxes read as things: a
notice board or a shelf is a sheet with its top and sides, a low pile of junk
lies on the floor, a heap of crates has its depth, as in the donor's viewer.

## Context

- The owner's screenshots from the air (2026-10-20, before 033):
  `runs/035-donor-things/owner-*.png`. A notice board leans as fanned planes
  (it should be two planes: the top and the sides); heaps of crates and junk
  looked better in the donor's viewer; a bed shows a corner sticking out
  behind it (`owner-31.png`, right of the headboard).
- The donor decides how each thing stands (`vendor/fp/things.py`, `stand`):
  a sheet along an axis for thin things (shelves, boards, counters), a card,
  or flat on the floor for low piles; `props.py` `groups`/`compose` put back
  together things the artists cut into several objects. Our live objects
  ignore that choice and are all cards; find what reaches `scene.json` and
  use it.
- Start by comparing the owner's spots in the donor's viewer and ours.

## Acceptance

- The owner's spots, from the air, before | after in `runs/035-donor-things/`;
  the bed's corner gone.
- Regression 16/16, or changed references with before | after.

## Out of scope

Walls (034). Real 3D models (M2).

## Log
