---
id: 011
title: Klamath gallery and problem list
milestone: M1
status: doing
owner: claude
suggested: gpt
claude-only: no
user-check: no
depends-on: [003, 010]
reviewed: no
---

## Goal

A complete picture of what is wrong in 3D on every Klamath map, so task 012
fixes causes, not single spots.

## Context

The regression tool (task 003) takes still snapshots at a chosen hex. The 2D
picture of the same spot is the reference for what must be visible and in
which order. 2D: run without `--dev-hero-preview` / `--dev-walk-preview`.

## Acceptance

- Regression spots for every Klamath map and elevation: entrances, every
  building, doors, furniture, people, map edges; references committed.
- `runs/011-klamath-gallery/`: for each spot a 2D and a 3D snapshot side by
  side, and `README.md` listing each spot with what is wrong in 3D: draw
  order (something in front that should be behind), missing or extra walls,
  holes in the floor, objects floating or sunk, wrong transparency, anything
  else. One line per problem, with the spot name and the object if known.
- Problems that look alike are grouped.

## Out of scope

Fixing anything.

## Log
