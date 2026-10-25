# Task 025 handoff, 2026-10-24

## Current status

- Task 025 is done and reviewed. Only small green NPC conversation replies and player answers use sharp text; menus, titles, headers, Pip-Boy and every other UI font remain pixel-rendered.
- The decoder supports canonical English and Russian settings, case-insensitively. Unsupported, empty and unknown settings preserve pixel text before restoration or drawing.
- PR #77 is owned by the parent for CI, readiness and merge.
- Latest source is `d1677a9b` on `task/025-sharp-text`. The captured executable SHA-256 is `0f6b2d1826a007bac9aa710a1edbf37dffc569c937f10f0164f3c4c2936f8d30`.
- Worktree: `E:/Projects/fallout2-ce-3d-gpt`. Do not switch branches in `E:/Projects/fallout2-ce-3d` or touch its owner executable or window.
- Only `tools/dev-snapshot.ps1` may start `fallout2-ce-test.exe`. Check for the shared tester and another worker launcher before every snapshot. Stop only the tester started by this helper.
- Preserve untracked task017 evidence, old failed task025 captures and local build folders. Stage only explicitly selected evidence files.

## Accepted checks

- Official Release full clean build after production source `361694e0` completed 719 targets. For `d1677a9b`, `git_version.h` was regenerated and its three consumers were forced to rebuild in the official MSVC environment. Full CTest passed 14/14.
- The current Russian x3 dialog fixture is `runs/025-sharp-text/cp1251-extents-20261024/d1677a9b-ru-x3-dialog-on/`. Root visually confirmed the breve in Ё and Й is visible, crisp and inside the small line; HUD and buttons remain original. Metadata: 12 runs, 420 glyphs, 5,537 restored pixels, 4.883 ms.
- The current English x2 cursor fixture is `runs/025-sharp-text/cp1251-extents-20261024/d1677a9b-en-x2-cursor-on/`. At actual opaque engine cursor pixel (673,533), decoded RGB `(84,84,84)` matches palette and overlay alpha changes `4->0`. At transparent pixel (700,534), text alpha remains `46->46`; same-run ink is 1,151 before and 1,076 after masking. Metadata: 8 runs, 337 glyphs, 1,887 restored pixels, 2.108 ms.
- The sharp-off regression passed all 16 spots with zero changed pixels, `update:false`, source `d1677a9b` and the same EXE hash. Summary: `runs/025-sharp-text/final-validation-20261024-regression-summary.json`.
- The prior English/Russian x2/x3 NPC matrix from `6a4a86ed` remains useful for fixture/off behavior, but its sharp-on font size predates the vertical extent correction. The current two snapshots replace it for sharp-on visual acceptance. Earlier pixel-identical menu pairs remain valid because menus do not use the sharp renderer.
- Renderer tests cover CP1251 diacritics at the real font-101 line height, x2/x3 line-mask clipping and stable per-face fitting. Locale tests reject French, German, Italian, Spanish, empty and unknown values. CPU tests cover clipped scrolling, owner witnesses and palette fades. Exporter tests passed 44/44 and regression unit tests passed 12/12 before this raster-only change.
- The inspected live Sulik reply had page offset 0. Do not claim runtime paging acceptance.

## Superseded or rejected evidence

- The prior sharp-on NPC captures under `runs/025-sharp-text/npc-dialog-20261024/committed-*-6a4a86ed/` are superseded for glyph size by the two current-source captures above; their off snapshots remain valid.
- The previous cursor fixture from `5a8c0183` is superseded by the current `d1677a9b` cursor check.
- `runs/025-sharp-text/cursor-runtime-20261024/2af21dea-en-x2/` is rejected because the engine cursor remained at screen center.
- Earlier all-UI sharp-text notes are superseded by the narrow conversation-only scope in `tasks/025-sharp-text.md`.