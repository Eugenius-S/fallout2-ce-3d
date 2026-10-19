---
id: 012
title: Correct scenes on all Klamath maps
milestone: M1
status: doing
owner: claude
suggested: claude
claude-only: yes
user-check: yes
depends-on: [011]
reviewed: no
---

## Goal

Every Klamath map looks right in 3D: whatever the 2D game shows in front is in
front, nothing missing, nothing extra.

## Context

The problem list in `runs/011-klamath-gallery/README.md`. Fix the conversion
rules (exporter and scene), not single objects; record each rule in the
roadmap's architecture notes only if another task depends on it.

## Acceptance

- Every problem in the 011 list is fixed or has a written reason why not and a
  follow-up task.
- Regression references updated with before and after images in the PR.
- The owner walked the Klamath maps and accepted them.

## Out of scope

Roofs (013), light.

## Log

### 2026-10-19 claude
Found by the owner in the game (KLADWTWN, after roofs, task 013), to fix here:
- `runs/012-klamath-scene-correctness/owner-wall-corner.png`: at the corner of
  a dark building a vertical strip of the wall art stands off the corner (a
  slab end or corner post drawn on the front plane).
- `runs/012-klamath-scene-correctness/owner-roof-corner.png`: the roof's
  corner and the wall top below it do not line up; the wall top runs on
  past the roof edge.
