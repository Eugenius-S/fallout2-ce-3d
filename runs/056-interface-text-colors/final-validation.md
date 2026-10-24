# Task 056 validation

Date: 2026-10-24

## Runtime captures

All eight captures use Release source `0e203738` and EXE SHA-256 `c0a5fbe34fa299ba2cb61f968e2e5e17407e567d150c089b0f293614e7329654`.

| Screen | Language and scale | Sharp text | Evidence |
| --- | --- | --- | --- |
| Inventory | Russian, 1920x1440, x3 | On | `accepted-20261024/ru-x3-inventory-on/` |
| NPC dialogue | Russian, 1920x1440, x3 | On | `accepted-20261024/ru-x3-dialog-on/` |
| Character | Russian, 1920x1440, x3 | On | `accepted-20261024/ru-x3-character-on/` |
| Lower HUD message log | Russian, 1920x1440, x3 | On | `accepted-20261024/ru-x3-log-on/` |
| Barter | English, 1920x1080, x2 | On | `accepted-20261024/en-x2-barter-on/` |
| Inventory | Russian, 1920x1440, x3 | Off, original pixel text | `accepted-20261024/ru-x3-inventory-off/` |
| NPC dialogue | Russian, 1920x1440, x3 | Off, original pixel text | `accepted-20261024/ru-x3-dialog-off/` |
| Character | Russian, 1920x1440, x3 | Off, original pixel text | `accepted-20261024/ru-x3-character-off/` |

Decoded PNG and prepared overlay checks confirmed opaque interface green `#3FB419` at physical `(1141,121)` in the character view, with a separate black description-text sample `(0,0,0)` at `(1136,951)`. The lower HUD log sample is exact `#4BD51F` at `(384,1307)`. In barter, both real `$0` totals match the active white palette `(252,252,252)` at `(822,979)` and `(992,979)`. Sulik dialogue fixture output confirms a real Klamath conversation and the decoded dialog frame contains the primary green in the reply area. The three OFF captures report `setting=false`, `active=false`, `drawn=false` and preserve the original pixel-font presentation.

## Build and tests

- Official clean Release build: 721/721 targets.
- CTest: 14/14 passed.
- Scene exporter tests: 44/44 passed.
- Regression unit tests: 12/12 passed.
- Official sharp-off regression: all 16 spots passed with zero changed pixels; summary is `accepted-20261024/regression-16-summary.json`.

The small-font brightness request is addressed by the requested color update. The independent black dialogue background and Russian custom-menu localization findings remain in the inbox.
