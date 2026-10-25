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

Use the owner's corrected font-101 default weight: IBM Plex Mono Bold 700. Keep the selected global size at 1.29 and the main inventory/PipBoy size at 1.22. Fix the reported font treatment in injury labels, PipBoy underlines, and floating character barks.

## Context

- The previous task applied SemiBold 600 based on an earlier selection. The owner has now explicitly corrected that choice to Bold 700.
- Keep the Bold/SemiBold preview override available for future comparison.
- Preserve all shipped size scopes, SPECIAL-label centering, plaque lifts, dialogue spacing, colors, and SharpText-off pixels.
- The owner reports inactive injury labels too dark, PipBoy underlines misaligned with the larger glyphs, floating character barks retaining pixel outlines, and live-owner text appearing smaller than reviewed captures. The first three are bounded rendering fixes. The live-owner size difference is investigated as a source/config mismatch and must not trigger an unrequested restart.

## Acceptance

- Eligible font-101 runs use Bold 700 by default at 1.29, except tagged main-inventory and PipBoy runs, which use Bold 700 at 1.22.
- Explicit per-window face preview overrides still select either embedded Bold or SemiBold.
- Inactive, unselected injury labels use `COLOR_DARK_GREEN`; selected, active, and non-green text retain their existing colors.
- Modern PipBoy underlines follow the visible glyph ink bounds with a one-logical-pixel gap and matching horizontal span; SharpText-off pixels remain unchanged.
- Modern floating character barks render their outline from the sharp glyph mask and restore the legacy morphology pixels during replacement/removal; SharpText-off remains unchanged.
- Live-owner comparison records executable hash, source, resolution, and scale, without closing, focusing, or restarting the owner's game.
- Fixtures and tests report/assert the effective Bold default while retaining explicit SemiBold override coverage.
- Release build, CTest, Python exporter/regression suites, and SharpText-off regression pass without updating references.
- Source-bound inventory, PipBoy, character, and dialogue captures show the requested Bold default; original font pixels remain unchanged when SharpText is off.

## Out of scope

- Changing font sizes, dialogue spacing, legacy coordinates, public settings, or font resources.

## Log

### 2026-10-25 gpt
Done: claimed the correction task on `task/074-bold-font-default-correction` from `origin/main` at `8489e442`.
Next: change the selected face default to Bold 700, update focused assertions and fixture output, then build and capture.
Check: task `074` is the first free ID after `073-glow-ladder`; no remote task/074 branch exists.

### 2026-10-25 gpt
Done: selected Bold 700 by default, added modern-only injury-status color role, aligned PipBoy underlines to visible glyph ink, and added floating-bark outline capture, restoration, and padded-layout support. SharpText-off source colors, legacy outlines, line placement, and object placement remain on the original path.
Next: commit and push this checkpoint, force source-stamp consumers to rebuild, then capture the three new visual scopes at owner resolution and finish the remaining regression/runtime evidence.
Check: clean official MSVC Release build passed all 733 targets; CTest passed 15/15. Focused tests cover injury palette fades, underline span/gap/clipping at E1.29 and D1.22 with x2/x3, exact outline-pixel restoration/copying, and multiline bark layout.
