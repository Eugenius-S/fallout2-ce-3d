---
id: 205
title: Fly-tour evidence for flight/turned-view renderer changes, starting with the quadrant sea fill (#320)
milestone: M1
status: todo
owner: glm
suggested: codex (P1 on #320)
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Every renderer change that touches the flight/turned views gets the
main-versus-change fly tour as its acceptance evidence, inspected by
eye, before the PR asks for a merge. The game-camera still gate cannot
stand in for this class of change: stills never draw the flight/turned
passes. First consumer: the quadrant sea fill on #320 (task 202).

## Context

- Codex P1 on #320 (2026-11-06): the change affects flight and turned
  views, precisely the views the cited regression spots do not
  exercise; the committed evidence held only the sfdock probes, and no
  tour comparison existed for the change.
- The tool exists: tools/tour/tour.py (task 078), the fly tour. Every
  spot of tools/tour/spots.json (294 spots, 130 maps) seen from the
  fly camera at each turn, before | after sides, changed.jpg sheets
  with a red frame round each changed pair.
- An engine change compares two checkouts, not scene caches: the floor
  extension is drawn by the build, not the export. The main side is a
  checkout of origin/main with a Release build (the plain main checkout
  is untrusted as a side; check its SHA first, the glm-main worktree
  can serve).
- Expected differences on the #320 run: water-corner maps gain sea
  past the corner (the fill working), sfdock's horizon sliver. Every
  other difference is pinned (spot, turn, what and why) before it
  counts as a defect.

## Acceptance

- The tour run for #320 (branch build vs origin/main build) under
  runs/205-fly-tour-evidence/: the sheets, changed.jpg, and a README
  naming each difference with its verdict (expected fill, or a find).
- Every unexpected difference pinned and either fixed or handed to its
  batch or the inbox with the evidence link.
- One standing line in AGENTS.md's build/check section: a renderer
  change to the flight/turned passes runs tools/tour/tour.py as
  acceptance evidence.

## Out of scope

The regression harness (203), the extension pass code itself (202's
remaining items), new spots in the tour list.

## Log

### 2026-11-06 glm
Cut from the codex P1 on #320. The board audit of gpt's finds
(2026-11-06) found every earlier one already sliced (126/127/140/141/
145/146/147/148/151/152/153/154/156/184 on the board; the codex P2s on
#303/#305 closed in 168/202; the codex note on #211 is 149). This is
the one open find.
