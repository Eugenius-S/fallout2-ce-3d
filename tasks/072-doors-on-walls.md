---
id: 072
title: Doors sit on the 3D wall planes
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

Every door is pinned to its 3D wall's plane, so from a turned view it stays
in the wall instead of floating as a card.

## Context

- The owner asked for it on 2026-10-24. The PLANE mechanism from 037 pins
  signs to walls already; doors need the same, with their open and closed
  frames.

## Acceptance

- Turned shots of a few doors, open and closed, under
  `runs/072-doors-on-walls/`.
- The regression stays where it is.

## Out of scope

Wall geometry (071).

## Log
