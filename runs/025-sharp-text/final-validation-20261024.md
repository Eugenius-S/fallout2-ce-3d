# Task 025 final validation, 2026-10-24

## Scope

Only small green NPC conversation replies and player answers use sharp text. All menus, titles, headers, Pip-Boy, inventory, character screens, barter, message log and floating text stay pixel-rendered. The feature accepts English and Russian settings only, case-insensitively. Unsupported, empty and unknown language settings preserve the original pixel text before restoration or drawing.

## Runtime evidence

- Real Klamath Sulik conversations were captured with sharp text off/on in English and Russian at x2 and x3. The eight snapshots bind to production source `6a4a86ed` and EXE SHA-256 `3173879bcdae46afd9ee328d0e2be5bbe5cc0921e981f46bd6d9dcd86f61412c`; evidence is under `runs/025-sharp-text/npc-dialog-20261024/committed-{en-x2,ru-x2,ru-x3,en-x3}-{off,on}-6a4a86ed/`.
- English x2 and Russian x3 Top menu off/on pairs are pixel-identical, with sharp text inactive and zero captured or restored runs when enabled. Evidence is under `runs/025-sharp-text/menu-scope-80d57da5/`.
- The current accepted cursor fixture uses a real Sulik reply and the engine cursor. Its x2 1920x1080 snapshot binds to production source `361694e0` and EXE SHA-256 `db32ec45c9797ab8b1c654fe36cf4fe8f2bde2025e97327be915fd44d2f5452e`. Decoded PNG pixel (610,530) is RGB `(84,84,84)`, matching the actual cursor palette; sharp overlay alpha changes `126->0`. At transparent pixel (637,533) inside the same cursor rectangle, alpha stays `5->5`; same-run ink stays visible at 2467 pixels after cursor masking. The snapshot reports active/drawn, 9 runs and 342 glyphs. Accepted files are `runs/025-sharp-text/cursor-runtime-20261024/361694e0-en-x2/`.

## Final checks

- Official Release full clean build after production source `361694e0` completed all 719 targets. Full CTest passed 14/14 after the owner-witness test update in `d8dcb168`.
- The final sharp-off regression passed all 16 spots with zero changed pixels and `update: false`. All 16 capture JSON files bind to production source `361694e0` and the same EXE SHA-256 `db32ec45c9797ab8b1c654fe36cf4fe8f2bde2025e97327be915fd44d2f5452e`. Curated summary: `runs/025-sharp-text/final-validation-20261024-regression-summary.json`.
- Scene exporter tests passed 44/44 and regression unit tests passed 12/12. Locale tests accept English/Russian and reject French, German, Italian, Spanish, empty and unknown values. CPU tests cover clipped scrolling, owner-witness validity and palette fades.
- The inspected live Sulik reply had page offset 0, so runtime paging is not claimed.
- Cursor evidence from `5a8c0183` is superseded by the source-bound `361694e0` fixture above. The earlier `2af21dea` cursor artifact remains rejected because the engine cursor stayed at screen center.

PR #77 remains under parent ownership for CI, readiness and merge.
