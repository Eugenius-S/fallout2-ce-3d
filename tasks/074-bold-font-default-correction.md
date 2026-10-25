---
id: 074
title: Correct font-101 default weight to Bold 700
milestone: M1
status: doing
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: [057]
reviewed: no
---

## Goal

Use the owner's corrected font-101 default weight: IBM Plex Mono Bold 700. Keep the selected global size at 1.29 and the main inventory/PipBoy size at 1.22.

## Context

- The previous task applied SemiBold 600 based on an earlier selection. The owner has now explicitly corrected that choice to Bold 700.
- Keep the Bold/SemiBold preview override available for future comparison.
- Preserve all shipped size scopes, SPECIAL-label centering, plaque lifts, dialogue spacing, colors, and SharpText-off pixels.

## Acceptance

- Eligible font-101 runs use Bold 700 by default at 1.29, except tagged main-inventory and PipBoy runs, which use Bold 700 at 1.22.
- Explicit per-window face preview overrides still select either embedded Bold or SemiBold.
- Fixtures and tests report/assert the effective Bold default while retaining explicit SemiBold override coverage.
- Release build, CTest, Python exporter/regression suites, and SharpText-off regression pass without updating references.
- Source-bound inventory, PipBoy, character, and dialogue captures show the requested Bold default; original font pixels remain unchanged when SharpText is off.

## Out of scope

- Changing font sizes, geometry, colors, dialogue spacing, legacy coordinates, public settings, or font resources.

## Log

### 2026-10-25 gpt
Done: claimed the correction task on `task/074-bold-font-default-correction` from `origin/main` at `8489e442`.
Next: change the selected face default to Bold 700, update focused assertions and fixture output, then build and capture.
Check: task `074` is the first free ID after `073-glow-ladder`; no remote task/074 branch exists.
