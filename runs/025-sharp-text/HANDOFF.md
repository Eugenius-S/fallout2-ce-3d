# Task 025 handoff, 2026-10-24

## Current status

- Task 025 is done and reviewed. The accepted scope is only small green NPC conversation replies and player answers. Menus, titles, headers, Pip-Boy and every other UI font remain pixel-rendered.
- Sharp text supports canonical English and Russian settings, case-insensitively. Other, empty and unknown values preserve original pixel text before restoration or drawing.
- PR #77 is owned by the parent for CI, readiness and merge.
- Latest branch head is `d8dcb168`. Production source and runtime evidence are bound to `361694e0`; `d8dcb168` changes only test expectations and task notes.
- Worktree: `E:/Projects/fallout2-ce-3d-gpt`, branch `task/025-sharp-text`. Do not switch branches in `E:/Projects/fallout2-ce-3d` or touch its owner executable or window.
- Only `tools/dev-snapshot.ps1` may start `fallout2-ce-test.exe`. Check for both the shared tester and another worker's launcher before every snapshot. Stop only the tester started by this helper.
- Preserve untracked task017 evidence, old failed task025 captures and local build folders. Stage only explicitly selected evidence files.

## Accepted checks

- The official Release full clean build after production source `361694e0` completed all 719 targets. Full CTest passed 14/14 after the strict owner-witness tests in `d8dcb168`.
- English and Russian Klamath Sulik conversation off/on captures cover x2 and x3. English x2 and Russian x3 Top menu pairs are pixel-identical. Their evidence and source hashes are listed in `runs/025-sharp-text/final-validation-20261024.md`.
- The current English x2 cursor proof is `runs/025-sharp-text/cursor-runtime-20261024/361694e0-en-x2/`. At opaque engine cursor pixel (610,530), decoded PNG RGB `(84,84,84)` matches the cursor palette, and sharp overlay alpha changes `126->0`. At transparent pixel (637,533), sharp text alpha stays `5->5`; same-run ink is 2601 before and 2467 after cursor masking. Metadata reports active/drawn, 9 runs and 342 glyphs. EXE SHA-256: `db32ec45c9797ab8b1c654fe36cf4fe8f2bde2025e97327be915fd44d2f5452e`.
- The final sharp-off regression passed all 16 spots with zero changed pixels, `update: false`, source `361694e0` and the same EXE hash. Curated summary: `runs/025-sharp-text/final-validation-20261024-regression-summary.json`.
- Exporter tests passed 44/44; regression unit tests passed 12/12. Locale policy tests reject French, German, Italian, Spanish, empty and unknown values. CPU tests cover clipped scrolling, owner witnesses and palette fades.
- The inspected live Sulik reply had page offset 0. Do not claim runtime paging acceptance.

## Superseded or rejected evidence

- `runs/025-sharp-text/cursor-runtime-20261024/5a8c0183-en-x2-transparent-verified-v2/` was valid but is superseded by the current `361694e0` cursor fixture.
- `runs/025-sharp-text/cursor-runtime-20261024/2af21dea-en-x2/` is rejected because the engine cursor remained at screen center; its alpha assertion did not prove a cursor blit at the target.
- `runs/025-sharp-text/cursor-runtime-20261024/1c00e08b-en-x2-validated/` was a valid earlier proof but is superseded by later transparent-cell checks.
- Earlier all-UI sharp-text notes are superseded by the narrow conversation-only scope in `tasks/025-sharp-text.md`.
