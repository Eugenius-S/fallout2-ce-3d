# Final runtime captures

Captured on 2026-10-25 from source `a9674c33`, EXE SHA-256 `8c5b57d5754e491c41509dc422d264178f93a2f145b3a9a7b1cb72d6c992679f`. Each fixture directory keeps its original PNG, snapshot JSON, and process logs. Images are copied without resizing or editing.

| Capture | Language | Resolution | Scale | SharpText | Evidence |
| --- | --- | ---: | ---: | --- | --- |
| `character-off/` | Russian | 1920x1440 | 3 | Off | Character plaque control |
| `character-on/` | Russian | 1920x1440 | 3 | On | 91 runs, 761 glyphs, eight SPECIAL box runs |
| `inventory-off/` | Russian | 1920x1440 | 3 | Off | Pixel-identical to task 056 reference |
| `inventory-on/` | Russian | 1920x1440 | 3 | On | 36 runs, 218 glyphs |
| `dialogue-off/` | Russian | 1920x1440 | 3 | Off | Pixel-identical to task 056 reference |
| `dialogue-on/` | Russian | 1920x1440 | 3 | On | 13 runs, 422 glyphs |
| `hud-on/` | Russian | 1920x1440 | 3 | On | Real date log, lower HUD green `75,213,31` |
| `pipboy-on/` | Russian | 1920x1440 | 3 | On | 2 runs, 12 glyphs |
| `barter-en-x2-on/` | English | 1280x960 | 2 | On | Two white `$0` totals |
| `character-en-x2-boxed-label/` | English | 1280x960 | 2 | On | Eight SPECIAL box runs |

The Russian OFF character capture is also pixel-identical to the task 056 reference. Comparisons decode both images as RGBA and report exact equality:

- Inventory: `runs/057-interface-font-size-preview/accepted/final/inventory-off/scr00103.png` and `runs/056-interface-text-colors/runtime/ru-x3-inventory-off-b99d12c3/scr00087.png`.
- Dialogue: `runs/057-interface-font-size-preview/accepted/final/dialogue-off/scr00105.png` and `runs/056-interface-text-colors/runtime/ru-x3-dialog-off-b99d12c3/scr00088.png`.
- Character: `runs/057-interface-font-size-preview/accepted/final/character-off/scr00101.png` and `runs/056-interface-text-colors/runtime/ru-x3-character-off-b99d12c3/scr00089.png`.

All Russian x3 character fixture logs report font-101 line height `10..10` logical pixels. The corrected character plaque captures use the scoped two-pixel modern overlay lift; SharpText OFF remains pixel-identical to the task 056 baseline.
