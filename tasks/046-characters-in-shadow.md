---
id: 046
title: Characters darken in building shadows; painted shadows give way
milestone: M1
status: todo
owner: none
suggested: gpt
claude-only: no
user-check: yes
depends-on: [045]
reviewed: no
---

## Goal

A character or a sprite standing in a building's shadow darkens as a whole,
and the shadows painted into the art do not double the real ones.

## Context

- One shadow lookup at the sprite's anchor slightly above the ground,
  smoothed over about 150 ms, so crossing a shadow border is a short fade.
  Ambient light and the lamps (020) still light it.
- The exporter already gives the shadow-free sprite of each such board
  (`shadows`: board, sprite without the painted shadow, shadow sprite) and the
  floor shade (`casts`). With shadows on, use the shadow-free sprites and drop
  `casts`; with them off, the originals.

## Acceptance

- Snapshots in `runs/046-characters-in-shadow/`: the hero inside and outside
  a shadow, and a board with a painted shadow, shadows on and off.
- With shadows off the regression passes as it is.

## Out of scope

Cloud shadows (023).

## Log
