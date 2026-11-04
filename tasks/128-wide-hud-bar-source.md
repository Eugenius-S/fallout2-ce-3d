---
id: 128
title: Wide-screen art, the HUD bar and the 4:3 full-screen art, with licenses
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

- Part C of the UI batch (193): taken together with 193 in one PR.
- Batch: the wide art (2026-11-04 inbox sweep). Step 2 folded in, the
  inbox line of 2026-10-27: the 4:3 full-screen art (main menu, splash and
  loading screens, death and ending slides) extended to 16:9 and 21:9. The
  owner's method: regenerate the art wider from the original, downscale to
  the game's pixel size, dither to match, then lay the untouched original
  over the middle with a small gradient mask at its edges. Step 1 (the HUD
  bar source and license) comes first; step 2 is its own PR.
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
