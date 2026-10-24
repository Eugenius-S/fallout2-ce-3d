# Task 056 validation

Date: 2026-10-24

## Current source-bound runtime captures

All eight captures use Release source `b99d12c30ec41cb1542331b31c29b0bf53ad7456` and EXE SHA-256 `bab1bf663dfe129704db162b25f379bee4c372e6ef8230c5221d0899dfd7554d`.

| Screen | Language and size | Sharp text | Evidence |
| --- | --- | --- | --- |
| Inventory | Russian, 1920x1440, x3 | On | `accepted-b99d12c3-20261024/ru-x3-inventory-on/` |
| NPC dialogue | Russian, 1920x1440, x3 | On | `accepted-b99d12c3-20261024/ru-x3-dialog-on/` |
| Character | Russian, 1920x1440, x3 | On | `accepted-b99d12c3-20261024/ru-x3-character-on/` |
| Lower HUD message log | Russian, 1920x1440, x3 | On | `accepted-b99d12c3-20261024/ru-x3-log-on/` |
| Barter | English, 1920x1080, x2 | On | `accepted-b99d12c3-20261024/en-x2-barter-on/` |
| Inventory | Russian, 1920x1440, x3 | Off, original pixel text | `accepted-b99d12c3-20261024/ru-x3-inventory-off/` |
| NPC dialogue | Russian, 1920x1440, x3 | Off, original pixel text | `accepted-b99d12c3-20261024/ru-x3-dialog-off/` |
| Character | Russian, 1920x1440, x3 | Off, original pixel text | `accepted-b99d12c3-20261024/ru-x3-character-off/` |

The on captures report sharp text active and drawn; the off captures report setting, active, and drawn all false. Decoded PNG pixels confirm opaque primary green `#3FB419` in inventory and character, HUD log green `#4BD51F`, black description text `(0,0,0)`, and both barter `$0` totals matching the live white palette `(252,252,252)`. The real Sulik dialogue fixture passed and its reply area contains 1,988 pixels of `#3FB419`.

## Fade behavior fix and checks

The renderer now derives the transition from the canonical source palette color to its current live RGB channels, then applies that transition to the target green. It preserves white and the tested red/blue endpoints, and handles the tested intermediate fade. Font renderer tests check both green roles at normal intensity, reduced source intensity, half-to-black, white endpoint, midpoint-to-white, black endpoint, and red and blue endpoints. Existing non-green, antialias, and palette-state tests still pass.

- Official MSVC incremental build: 15/15 targets; embedded source hash is `b99d12c3`. Build log: `build/palette-fade-b99d12c3.log`.
- Embedded fonts test executable: passed.
- CTest: 14/14 passed.
- Official sharp-off regression: all 16 spots passed with zero changed pixels; summary is `accepted-b99d12c3-20261024/regression-16-summary.json`.

The earlier clean build and exporter/regression-unit suites passed on source `0e203738`, before this renderer-only fade correction. The three git-version consumers plus `text_draw.cc` and `ui_fixture.cc` were explicitly rebuilt. The earlier 0e runtime set remains in `accepted-20261024/` as historical evidence; this section records only current b99 captures.
