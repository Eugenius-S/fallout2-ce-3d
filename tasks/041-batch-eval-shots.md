---
id: 041
title: Shoot the shapes eval in one game run per map
milestone: M1
status: done
owner: claude
suggested: claude
claude-only: no
user-check: no
depends-on: [036]
reviewed: yes
---

## Goal

The shapes eval's views come fast enough to shoot all of Klamath (task 039),
without sound from the background test runs.

## Context

- `eval.py shoot` started the test instance once per view: about 15 s each,
  45 minutes for the 40 eval FRMs, hours for Klamath's 465.

## Acceptance

- One test-instance run per map elevation; the views are the same as before.
- The background test runs make no sound.

## Out of scope

Comparing sheets with the last run, finding vanishing things by script.

## Log

### 2026-10-20 claude
Done: a `--dev-map-tour` stop may carry `:solo:turn:distance` after its view
tile: walls.bin is read again with only the things within `solo` hexes
(`resoloStaticWalls`, a fraction of a second), the fly camera takes off and
circles `turn` degrees at `distance` units, and the stop's snapshot is taken.
`eval.py shoot` makes one such tour per map elevation (passed through
PowerShell `-Command`: through `-File` a list with commas becomes an array).
All 40 eval FRMs: 192 s instead of about 45 minutes, the same views.
`dev-snapshot.ps1` gives the test instance SDL's dummy audio driver: no
sound device, no hiss, whatever `game/fallout2.cfg` says.
Next: 039 can shoot all of Klamath this way.
Check: `py -3 tools/shapes/eval/eval.py shoot` (run it from Bash: launched
from a PowerShell tool session it failed at once, cause not found).
