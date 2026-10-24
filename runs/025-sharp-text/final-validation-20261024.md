# Task 025 final validation, 2026-10-24

## Scope

Only small green NPC conversation replies and player answers use sharp text. Menus, titles, headers, Pip-Boy, inventory, character screens and all other UI remain pixel-rendered. The renderer supports canonical English and Russian settings, case-insensitively; unsupported, empty and unknown settings keep original pixel text before restoration or drawing.

## Runtime evidence

- Real Klamath Sulik conversations were captured with sharp text off/on in English and Russian at x2 and x3. The eight snapshots bind to source `6a4a86ed` and executable SHA-256 `3173879bcdae46afd9ee328d0e2be5bbe5cc0921e981f46bd6d9dcd86f61412c`; evidence is under `runs/025-sharp-text/npc-dialog-20261024/committed-{en-x2,ru-x2,ru-x3,en-x3}-{off,on}-6a4a86ed/`.
- English x2 and Russian x3 Top menu off/on pairs are pixel-identical, with sharp text inactive and zero captured/restored runs when enabled. Evidence is under `runs/025-sharp-text/menu-scope-80d57da5/`.
- The final cursor fixture uses a real Sulik reply and the engine cursor. Its 1920x1080 x2 snapshot binds to source `5a8c0183` and executable SHA-256 `fa5b4c618a35c4436ff7857f2218105f2994454143477236081b8df88005582e`. At opaque physical pixel (610,530), palette RGB and decoded PNG RGB both equal `(84,84,84)` and sharp overlay alpha changes `126->0`. At transparent pixel (637,533), inside the same cursor rectangle, sharp text alpha stays `5->5`; same-run ink is `2601->2467`. The snapshot reports active/drawn, 9 runs and 342 glyphs. Accepted artifacts: `runs/025-sharp-text/cursor-runtime-20261024/5a8c0183-en-x2-transparent-verified-v2/`.

## Final checks

- Official Release build embeds source `5a8c0183`; full CTest passed 14/14.
- Final sharp-off regression passed 16/16 spots with zero changed pixels. Summary: `runs/regression/20261024-111711-398-28976/summary.json`. All captures use EXE SHA-256 `fa5b4c618a35c4436ff7857f2218105f2994454143477236081b8df88005582e`.
- Scene exporter tests passed 44/44 and regression unit tests passed 12/12. Locale tests accept English/Russian and reject French, German, Italian, Spanish, empty and unknown values.
- Focused CPU tests cover clipped scrolling and palette fades. The inspected live Sulik reply had page offset 0, so runtime paging is not claimed.
- Earlier cursor artifact `2af21dea` is rejected because the engine cursor remained at screen center. The later `1c00e08b` proof was valid but is superseded by the final cursor-over-transparent-text evidence above.

PR #77 remains under parent ownership for CI, readiness and merge.
