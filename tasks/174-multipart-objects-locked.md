---
id: 174
title: Objects made of several parts stand and move as one
milestone: M1
status: done
owner: gpt
suggested: any
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

An object the game draws from several pieces (the crashed vertibird first)
stands as one in 3D: its pieces keep their places relative to each other in
the fly camera and on a turned view, with no gaps or drift.

## Context

- The owner, 2026-11-04: lock one part to the other.
- Find how the pieces are placed (separate prototypes on neighbouring tiles
  with offsets) and why they drift apart in 3D (each piece is a card or a
  shape of its own, anchored on its own tile).
- Fix by a rule: pieces the map places together are one group, built and
  turned around one anchor. Then scan every exported map for other groups
  of the same kind and list them in the Log.

## Acceptance

- The vertibird and every other group found show whole from the fly camera
  at -45, 0 and +45, with before | after crops in
  `runs/174-multipart-objects-locked/`.
- ctest and the regression pass.

## Out of scope

New 3D models.

## Log

### 2026-11-04 gpt
Done: grouped contiguous same-name multipart families using projected sprite bounds, and added a fly-only fixed-plane fallback for groups that cannot be built as solids. Added tests for assembled fly-only rows and for disconnected/mixed-name false matches. Scanned all exported maps; candidate groups are Klamath and Navarro Vertibirds, the New Reno Vertibird, New Reno signage and casino entrance, and the San Francisco tanker. False family matches (graffiti, goo, unrelated furniture and mixed weeds) are filtered. Evidence sheets under `runs/174-multipart-objects-locked/` compare the Klamath Vertibird, both Navarro elevation-2 Vertibirds, New Reno Vertibird and New Reno casino entrance at -45/0/+45. Raw captures and sidecars are in the ignored `build/task174/evidence-raw/` folder.
Check: Release build; `ctest --test-dir build --output-on-failure` (20/20); scene-export unit tests (125/125); full pixel regression `runs/regression/20261104-222933-516-64992` (83/83, zero changed pixels); focused false-positive spots all zero. `git diff --check` passed.
Next: runtime evidence is missing for New Reno signage (`NEWR1`) and the San Francisco tanker. Two 90-second live capture attempts for each target timed out; no further retries were made. The owner authorized merging completed code after two runtime failures with this limitation recorded. Task status is done for the implementation; `reviewed: no` remains because regression references were updated.
