---
id: 068
title: Small things from the owner's review get their right shape
milestone: M1
status: todo
owner: none
suggested: claude
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The things the owner named in their live review of 2026-10-24 read right in 3D.

## Context

From `runs/046-characters-in-shadow/review-2026-10-24/` and their notes:

- The computer stands must get real 3D too.
- The notice standee (the A-frame board by the long building) has the wrong
  shape.
- Two buckets overlap; they should read as one.
- A straight card cutout that was good before is gone (which card is
  unconfirmed: compare the review shots with the 039 sheets).
- A floor tile that was 3D before shows as 2D again: find why it dropped out.

The shapes eval (`tools/shapes/eval/`, 039's `klamath.py score`) is the
check; each fix lands as a verdict there.

## Acceptance

- Each item has a before and after sheet under
  `runs/068-review-shapes/`, or a Log line on why it is art and stays.
- `py -3 tools/shapes/eval/klamath.py score` does not drop.

## Out of scope

Walls, corners and doors (071, 072); card order (067).

## Log

### 2026-10-25 claude
Folded into 075; close together with it.
