# Task 025 final validation, 2026-10-24

## Scope

Only small green NPC conversation replies and player answers use sharp text. All menus, titles, headers, Pip-Boy, inventory, character screens, barter, message log and floating text stay pixel-rendered. The feature accepts English and Russian settings only, case-insensitively. Unsupported, empty and unknown language settings preserve the original pixel text before restoration or drawing.

## Current-source runtime evidence

- The Russian x3 Klamath Sulik dialog snapshot uses production source `d1677a9b` and EXE SHA-256 `0f6b2d1826a007bac9aa710a1edbf37dffc569c937f10f0164f3c4c2936f8d30`. It is 1920x1440 physical / 640x480 logical at scale 3, with sharp text active/drawn, 12 runs, 420 glyphs, 5,537 restored pixels and 4.883 ms preparation. The real fixture confirms `Kcsulik.int`, speaker 26 at tile 23478, hero tile 25075 and `talk_result=0`. Evidence: `runs/025-sharp-text/cp1251-extents-20261024/d1677a9b-ru-x3-dialog-on/`.
- The English x2 cursor snapshot uses that same source and executable. It is 1920x1080 physical / 960x540 logical at scale 2, with sharp text active/drawn, 8 runs, 337 glyphs, 1,887 restored pixels and 2.108 ms preparation. At opaque engine cursor pixel (673,533), decoded PNG RGB `(84,84,84)` matches the cursor palette and sharp overlay alpha changes `4->0`. At transparent pixel (700,534), alpha stays `46->46`; same-run ink changes from 1,151 to 1,076 pixels. Evidence: `runs/025-sharp-text/cp1251-extents-20261024/d1677a9b-en-x2-cursor-on/`.

## Earlier evidence still in scope

- English x2 and Russian x3 Top menu off/on pairs remain pixel-identical. The metric change only affects the active TTF conversation renderer, so the original pixel menu proof remains valid. Evidence: `runs/025-sharp-text/menu-scope-80d57da5/`.
- The eight Sulik off/on snapshots from source `6a4a86ed` still document fixture setup and sharp-off behavior. Their sharp-on glyph size predates the CP1251 extent fix and is superseded by the current-source Russian x3 and English x2 captures above. Earlier menu and broad all-UI sharp-on notes do not define the accepted scope.

## Final checks

- Official Release full clean build after production source `361694e0` completed all 719 targets. For `d1677a9b`, the official configure script regenerated `git_version.h`; `game.cc`, `devtools.cc` and `mainmenu.cc` were forced through the same MSVC environment so the executable embeds the current source. Full CTest passed 14/14.
- The final sharp-off regression passed all 16 spots with zero changed pixels and `update: false`. All 16 capture JSON files bind to `d1677a9b` and EXE SHA-256 `0f6b2d1826a007bac9aa710a1edbf37dffc569c937f10f0164f3c4c2936f8d30`. Curated summary: `runs/025-sharp-text/final-validation-20261024-regression-summary.json`.
- Renderer tests use the production font-101 line height of 9. At x2 and x3 they verify that CP1251 И differs from Й and Е differs from Ё, and that the full render matches the captured-line mask. The fixed per-face metrics are derived from the complete printable decoded CP1251 repertoire. Locale tests accept English/Russian and reject French, German, Italian, Spanish, empty and unknown values. CPU tests cover clipped scrolling, owner witnesses and palette fades.
- Scene exporter tests passed 44/44 and regression unit tests passed 12/12 before this renderer-only correction. The inspected live Sulik reply had page offset 0, so runtime paging is not claimed.
- Cursor proof from `5a8c0183` and sharp-on NPC size evidence from `6a4a86ed` are superseded by the current captures. Cursor artifact `2af21dea` remains rejected because the engine cursor stayed at screen center.

PR #77 remains under parent ownership for final CI, readiness and merge.
