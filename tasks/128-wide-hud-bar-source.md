---
id: 128
title: Find the wide HUD bar art and its license
milestone: M1
status: todo
owner: any
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Know which Fallout 2 mod or patch ships the wide bottom interface bar on the
owner's screenshot, where to get it, and whether its license lets us ship
or reuse that art.

## Context

- The owner, 2026-11-01: `runs/inbox/2026-11-01-wide-hud-bar.webp`. The stock
  bar sits in the middle; metal side panels with a round vault-gear ornament
  fill the space to each screen edge.
- Likely candidates: the High Resolution Patch's wide interface art, sfall
  based packs, Fallout 2 Restoration Project, Fallout Et Tu. Check each.

## Acceptance

- A short report in the Log: the source (name, link, version), the files
  that hold the art (FRM names and sizes), the license or the author's terms
  quoted briefly, and a verdict: we may ship it, we may only load it from the
  player's own install, or we may not use it.
- No art committed to the repo.

## Out of scope

Putting the bar into the game. That is a new task after the verdict.
