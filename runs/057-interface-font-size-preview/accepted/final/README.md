# Final runtime captures

Captured on 2026-10-25 from source `a9674c33`, EXE SHA-256 `8c5b57d5754e491c41509dc422d264178f93a2f145b3a9a7b1cb72d6c992679f`. Each fixture directory keeps its original PNG, snapshot JSON, and process logs. Images are copied without resizing or editing.

| Capture | Language | Resolution | Scale | SharpText | Evidence |
| --- | --- | ---: | ---: | --- | --- |
| `character-off/` | Russian | 1920x1440 | 3 | Off | Character plaque control |
| `character-on/` | Russian | 1920x1440 | 3 | On | 91 runs, 761 glyphs, eight SPECIAL box runs |
| `inventory-off/` | Russian | 1920x1440 | 3 | Off | Inventory window exact; 432 lower HUD pixels differ from the latest task 056 frame |
| `inventory-on/` | Russian | 1920x1440 | 3 | On | 36 runs, 218 glyphs |
| `dialogue-off/` | Russian | 1920x1440 | 3 | Off | Pixel-identical to task 056 reference |
| `dialogue-on/` | Russian | 1920x1440 | 3 | On | 13 runs, 422 glyphs |
| `hud-on/` | Russian | 1920x1440 | 3 | On | Real date log, lower HUD green `75,213,31` |
| `pipboy-on/` | Russian | 1920x1440 | 3 | On | 2 runs, 12 glyphs |
| `barter-en-x2-on/` | English | 1280x960 | 2 | On | Two white `$0` totals |
| `character-en-x2-boxed-label/` | English | 1280x960 | 2 | On | Eight SPECIAL box runs |
| `inventory-off-repeat/` | Russian | 1920x1440 | 3 | Off | Byte-identical repeat of `inventory-off/` |

The dialogue and character OFF screenshots match the latest accepted task 056 frames exactly by RGBA byte comparison. The inventory screenshot has 432 differing pixels in a 60x9 physical-pixel rectangle at `(1218,1131)..(1277,1139)`, corresponding to logical `(406,377)..(425,379)` below the inventory window in the lower HUD. Its alpha channel is unchanged, all pixels outside this rectangle are identical, and no changed pixel uses the green font color. Both inventory fixture logs report 34 green font-101 runs and zero runs in other colors. A repeated capture from source `a9674c33` is byte-identical to this one. The current inventory image also matches the earlier task 056 `0e203738` image exactly; the same 432-pixel lower-HUD rectangle is the only difference between the two accepted task 056 inventory OFF images.

- Inventory: `runs/057-interface-font-size-preview/accepted/final/inventory-off/scr00103.png` vs `runs/056-interface-text-colors/accepted-b99d12c3-20261024/ru-x3-inventory-off/scr00087.png`; earlier exact frame: `runs/056-interface-text-colors/accepted-20261024/ru-x3-inventory-off/scr00079.png`.
- Dialogue: `runs/057-interface-font-size-preview/accepted/final/dialogue-off/scr00105.png` vs `runs/056-interface-text-colors/accepted-b99d12c3-20261024/ru-x3-dialog-off/scr00088.png`.
- Character: `runs/057-interface-font-size-preview/accepted/final/character-off/scr00101.png` vs `runs/056-interface-text-colors/accepted-b99d12c3-20261024/ru-x3-character-off/scr00089.png`.

All Russian x3 character fixture logs report font-101 line height `10..10` logical pixels. The corrected character plaque captures use the scoped two-pixel modern overlay lift. Dialogue and character OFF frames are pixel-identical to their latest task 056 references; the inventory window is unchanged and the documented lower HUD exception lies outside its text.
