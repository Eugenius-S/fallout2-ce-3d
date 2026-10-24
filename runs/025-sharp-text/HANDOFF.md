# Task 025 handoff, 2026-10-24

## Current status

- Task 025 is done and reviewed. The accepted scope is only small green NPC conversation replies and player answers. Menus, titles, headers, Pip-Boy and every other UI font remain pixel-rendered.
- Sharp text supports canonical English and Russian settings, case-insensitively. Other, empty and unknown values preserve original pixel text before restoration or drawing.
- PR #77 is owned by the parent for CI, readiness and merge.
- Worktree: `E:/Projects/fallout2-ce-3d-gpt`, branch `task/025-sharp-text`. Do not switch branches in `E:/Projects/fallout2-ce-3d` or touch its owner executable or window.
- Only `tools/dev-snapshot.ps1` may start `fallout2-ce-test.exe`. Check for both the shared tester and another worker's launcher before every snapshot. Stop only the tester started by this helper.
- Preserve untracked task017 evidence, old failed task025 captures and local build folders. Stage only explicitly selected evidence files.

## Accepted checks

- Source `5a8c0183` was configured and built with the official Release setup. Full CTest passed 14/14.
- English and Russian Klamath Sulik conversations were captured with sharp text off/on at x2 and x3. Menu off/on pairs are pixel-identical. Full evidence and metadata are listed in `runs/025-sharp-text/final-validation-20261024.md`.
- The final English x2 cursor proof is `runs/025-sharp-text/cursor-runtime-20261024/5a8c0183-en-x2-transparent-verified-v2/`. The decoded PNG at opaque engine-cursor pixel (610,530) is RGB `(84,84,84)`, matching the cursor palette, and sharp overlay alpha changes `126->0`. At transparent pixel (637,533), inside the same cursor rectangle, sharp text alpha stays `5->5`; same-run ink is `2601->2467`. Metadata reports active/drawn, 9 runs and 342 glyphs. EXE SHA-256: `fa5b4c618a35c4436ff7857f2218105f2994454143477236081b8df88005582e`.
- Final sharp-off regression passed all 16 spots with zero changed pixels on embedded source `5a8c0183`. Summary: `runs/regression/20261024-111711-398-28976/summary.json`.
- Exporter tests passed 44/44; regression unit tests passed 12/12. Locale policy tests reject French, German, Italian, Spanish, empty and unknown values. CPU tests cover clipped scrolling and palette fades.
- The inspected live Sulik reply had page offset 0. Do not claim runtime paging acceptance.

## Rejected evidence

- `runs/025-sharp-text/cursor-runtime-20261024/2af21dea-en-x2/` is rejected because the engine cursor remained at screen center; its alpha assertion did not prove a cursor blit at the target.
- `runs/025-sharp-text/cursor-runtime-20261024/1c00e08b-en-x2-validated/` was a valid earlier proof, superseded by the final opaque and transparent cell checks above.
- Earlier all-UI sharp-text notes are superseded by the narrow conversation-only scope in `tasks/025-sharp-text.md`.
