---
id: 057
title: Interface font size preview
milestone: M1
status: doing
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: [056]
reviewed: no
---

## Goal

Show five font-101 point-size variants on the Russian character screen for the owner to choose. Keep the regular scale at 1.00 and make larger scales available only through a dev-only preview.

## Context

- Current font 101 uses the embedded IBM Plex Mono Bold 700 face with size derived from the captured line height.
- The actual Russian character screen has 9 logical pixel text rows at 1920x1440 scale 3.
- Required variants are 1.00, 1.08, 1.15, 1.22, and 1.29. The preview may extend sharp glyph coverage within the live owner window while legacy drawing, accepted widths, line height, and wrapping stay unchanged.
- Keep source color behavior from task 056: normal interface green `#3FB419`, lower HUD log green `#4BD51F`, all other palette states unchanged.

## Acceptance

- The preview can be started only from the opt-in dev fixture and is off by default. No player setting, menu control, or changed default is added.
- Capture authentic character-screen frames at 1920x1440 scale 3 with identical game state. Keep the original 1.00, 1.08, and 1.15 frames from source `95700dbd`, and capture D=1.22 and E=1.29 from the same source revision and EXE SHA. Present original PNGs without resizing or editing their content.
- The 1.00 preview is pixel-identical to normal production rendering.
- Renderer tests cover `Й`, `Ё`, descenders, and other glyphs that meet the enlarged coverage edge. They verify no clipping at the selected preview scales and no overlay beyond the live owner window or captured text width.
- Review actual character rows for collisions with neighboring text at every variant. If a larger size needs a broader UI row-layout change, stop and report the limit instead of changing production layout.
- Release build and CTest pass.

## Out of scope

- Changing the default font size, adding a public setting, or merging a permanent size choice before the owner selects a variant.
- Changing the Bold 700 face, colors, brightness, UI layout, game font, or fonts other than 101.

## Log

### 2026-10-24 gpt
Done: claimed task 057 on the branch from origin/main.
Next: audit a bounded preview-only scale path, then implement after plan review.
Check: branch starts at `d64e12a6`; all tracked files were clean before branching.

### 2026-10-24 gpt
Done: added the opt-in character-screen preview at 1.00, 1.08, and 1.15 in commit `95700dbd`. Release build and CTest pass 14/14. Curated the normal 1.00 control and three variants in `runs/057-interface-font-size-preview/accepted/`. The normal control and explicit 1.00 PNGs decode pixel-identically. Visual review found no adjacent-row collisions or clipped diacritics in the 1.08 and 1.15 frames.
Next: owner chooses a preview size. Keep the production default at 1.00 until that choice.
Check: `runs/057-interface-font-size-preview/accepted/preview-report.md`, the matching snapshot JSON and fixture stdout in each variant folder, and CTest 14/14.

### 2026-10-24 gpt
Done: extended the preview-only allowlist with D=1.22 and E=1.29 in commit `d25c69e6`. Release build and CTest pass 14/14. Both new real character-screen captures use source `d25c69e6`, the same EXE SHA, and the same 1920x1440/x3 fixture setup. Curated the full PNG, snapshot JSON, and stdout under `runs/057-interface-font-size-preview/accepted/preview-1x22/` and `preview-1x29/`.
Next: owner chooses from 1.00, 1.08, 1.15, 1.22, and 1.29. Keep the production default at 1.00 pending that choice.
Check: the two new fixture logs confirm their requested scales; the renderer tests cover all five scales at x2 and x3.
