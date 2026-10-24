# Task 025 validation evidence, 2026-10-24

## Locale review follow-up

PR review found that CP1251 decoding must not run for unsupported Latin-script locales. Commit `5cde8f97` guards the renderer to English and Russian only; the language-policy unit test and full CTest pass. The presentation gate precedes text restoration and drawing, so rejected locales keep the original indexed text. Prior runtime and regression evidence below remains valid for supported English/Russian. No unsupported-locale runtime smoke is claimed; parent review is pending.

## Scope

Only small green NPC conversation replies and player answers use sharp text. Menus, titles, headers, Pip-Boy, inventory, character screens and other UI stay pixel-rendered.

## Runtime evidence

- English and Russian real Klamath conversations were captured with sharp text off/on at x2 and x3. The eight snapshots bind to source `6a4a86ed` and executable SHA-256 `3173879bcdae46afd9ee328d0e2be5bbe5cc0921e981f46bd6d9dcd86f61412c`; evidence is under `runs/025-sharp-text/npc-dialog-20261024/committed-{en-x2,ru-x2,ru-x3,en-x3}-{off,on}-6a4a86ed/`.
- English x2 and Russian x3 Top menu off/on pairs are pixel-identical, with sharp text inactive and zero captured/restored runs when enabled. Evidence is under `runs/025-sharp-text/menu-scope-80d57da5/`.
- The corrected cursor test uses a real Kcsulik reply and the engine cursor. The PNG at `runs/025-sharp-text/cursor-runtime-20261024/1c00e08b-en-x2-validated/scr00044.png` is 1920x1080 at scale 2. At physical pixel (610,530), expected and decoded RGB are both (84,84,84); the sharp overlay alpha changes from 126 to 0, while visible same-run ink changes from 2601 to 2408 pixels. The snapshot reports 7 runs, 281 glyphs, 1,812 restored pixels and 2.049 ms preparation. Its executable SHA-256 is `fdd9ed58c09d67c9e101d525b0f3471bfa587943a7574466c32e483dfb41f8f2`.

The earlier cursor screenshot from `2af21dea` is rejected: the cursor remained at screen center, so its alpha-mask assertion did not prove an engine cursor blit at the target.

## Final checks

- Source `1c00e08b`: sharp-off regression passed all 16 spots with zero changed pixels. Summary: `runs/regression/20261024-103814-975-9564/summary.json`.
- Full CTest: 14/14 passed.
- Scene exporter tests: 44/44 passed.
- Regression unit tests: 12/12 passed.
- Final audit: zero forbidden paths, zero scoped secret-pattern hits, zero added U+2014 characters, and clean `git diff --check`.

Focused CPU tests cover clipped scrolling and palette fades. The inspected live Kcsulik reply had page offset 0; no long live reply with a nonzero offset was observed, so runtime paging acceptance is not claimed.

Draft PR #77 is awaiting the parent's final CI update, readiness change and merge.
