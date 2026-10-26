---
id: 078
title: A fixed fly tour shoots every map before and after each scene change
milestone: M1
status: done
owner: claude
suggested: glm
priority: high
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

One command shoots a fixed set of turned camera views per Klamath map
(building corners, walls, roofs, furniture, at 0, -45 and +45 degrees) and
puts them on a before | after sheet, so an agent checks its own scene
change on one sheet before the owner looks.

## Context

- The owner, 2026-10-25: an automatic tour instead of their manual walk
  round the game; 071's corners took four rounds of the owner's eye.
- Building blocks: `tools/dev-snapshot.ps1` (`-FlyTurn`, `-FlyPitch`,
  `-FlyDistance`, `-ViewTile`, `-Map`), the shapes eval's one-run shooting
  and sheets (`tools/shapes/eval/eval.py`, 041 and 043), `tools/sheets`
  if it fits. One game run per map, as 041 does.
- Spots to start from: the kladwtwn long building corners (tiles 24268,
  24292, 20268, 20292, 20690), the bathhouse front (19265), Vic's shack
  (17467), the owner's spots in `runs/inbox/2026-10-25-review2-*.png`; add
  a few per other Klamath map. The spot list lives in one file.
- Turns stay within 45 degrees (077).

## Acceptance

- `tools/tour.ps1` (or the eval's command) shoots the tour for all seven
  maps and writes `before | after` sheets between two builds or caches;
  the sheets of one run are in `runs/078-fly-tour/`.
- AGENTS.md says a scene change attaches the tour sheet to its PR.

## Out of scope

Judging the views automatically.

## Log

### 2026-10-26 claude
Done: `py -3 tools/tour/tour.py --before SIDE [--after SIDE]` shoots every
spot of `tools/tour/spots.json` (43 spots on all seven Klamath maps and
their elevations: the 071 corners, the bathhouse, Vic's shack, the review2,
076 and 083 spots, the regression views) from the fly camera at -45, 0 and
+45 degrees, distance 8, one frozen (`-Still`) full-daylight run per map
elevation and side, and writes `before | after` sheets (one JPEG per map
elevation, `changed.jpg` with only the changed spots, red frames on changed
pairs, an "empty view" line for a spot off the map). A side is a built
checkout or a scene cache; `dev-snapshot.ps1 -SceneCache DIR` runs a cache
with this build. A full tour takes about 4.5 minutes per side; `--reuse`
keeps a side's shots. Without `-Still` wandering critters changed views
between two runs of one cache; with it main against main is 0 of 43.
The committed run, `runs/078-fly-tour/`: the scene cache of the exporter
before 081 against main's, the same build: 39 of 43 spots changed (roof
cuts and wall feet; klacanyn and two other spots without walls did not).
AGENTS.md: a scene change attaches the tour's `changed.jpg` to its PR.
Next: none; add spots to `spots.json` when the owner finds a new place.
Check: `py -3 tools/tour/tour.py --before <cache or checkout> --maps klagraz`.
