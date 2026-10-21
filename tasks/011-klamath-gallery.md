---
id: 011
title: Klamath gallery and problem list
milestone: M1
status: done
owner: claude
suggested: gpt
claude-only: no
user-check: no
depends-on: [003, 010]
reviewed: yes
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

### 2026-10-19 claude
Done: 62 2D | 3D views over the 11 reachable Klamath map elevations and the
problem list in `runs/011-klamath-gallery/README.md` (gray empty ground, no
map edge, no game light, building corners; no draw-order problems). Regression
spots for every Klamath map elevation (16 spots, references taken). New dev
tools: 2D snapshots go through the GL presentation (window size, no odd-width
problem) when the renderer runs, `--dev-classic` / `-Classic`, tour stops with
a view hex (`map:elevation:hex`), `tools/gallery_spots.py`,
`tools/gallery_pairs.py`.
Check: `tools/regression.ps1` (16 spots pass); the gallery pictures.
