# Final runtime captures

The original global-font rollout matrix was captured on 2026-10-25 from source `a9674c33`, EXE SHA-256 `8c5b57d5754e491c41509dc422d264178f93a2f145b3a9a7b1cb72d6c992679f`. Later scoped corrections are listed with their own source hashes below. Each fixture directory keeps its original PNG, snapshot JSON, and process logs. Images are copied without resizing or editing.

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
| `inventory-weight-off/` | Russian | 1920x1440 | 3 | Off | Source `73cfb968`; byte-identical to the earlier inventory OFF frame |
| `inventory-weight-on/` | Russian | 1920x1440 | 3 | On | Source `73cfb968`; 36 runs, 218 glyphs; only total-weight text receives the two-row modern lift |

The dialogue and character OFF screenshots match the latest accepted task 056 frames exactly by RGBA byte comparison. The inventory screenshot has 432 differing pixels in a 60x9 physical-pixel rectangle at `(1218,1131)..(1277,1139)`, corresponding to logical `(406,377)..(425,379)` below the inventory window in the lower HUD. Its alpha channel is unchanged, all pixels outside this rectangle are identical, and no changed pixel uses the green font color. Both inventory fixture logs report 34 green font-101 runs and zero runs in other colors. A repeated capture from source `a9674c33` is byte-identical to this one. The current inventory image also matches the earlier task 056 `0e203738` image exactly; the same 432-pixel lower-HUD rectangle is the only difference between the two accepted task 056 inventory OFF images.

The scoped inventory-weight correction uses source `73cfb968` and EXE SHA-256 `5174d457f2b1e3bfa5536ea38512fbf6bc6a98a7407ce9474eff9113826c2ac0`. Its SharpText OFF frame is byte-identical to the earlier task 057 inventory OFF frame. The ON fixture retains the same 1.29 SemiBold font and green run count; only the total-weight line's modern overlay moves upward by two logical pixels. The corresponding original and final frames are stored in `inventory-weight-off/` and `inventory-weight-on/`.

## Scoped dialogue spacing and compact font role

The dialogue-spacing pair uses source `dfaa8acc` and EXE SHA-256 `177081f182834b79872ee5724e4fe276606dbe0dd3c6c80cffc5cf9da7f8ee1f`. The compact-role pairs use source `eb2d3c4e` and EXE SHA-256 `5e4d1c6592071733dbd3f4f796fac670e8de3ae8aec4e2d41498063fcad2de73`. The dialogue-layout refactor smoke uses source `fe289ad6` and EXE SHA-256 `b2321b7c03e691c18d3babbd939641c0f5064ffbeb5218a2d17ae06b79adb20b`.

| Capture | Language | Resolution | Scale | SharpText | Source | Evidence |
| --- | --- | ---: | ---: | --- | --- | --- |
| `dialogue-layout-off/` | Russian | 1920x1440 | 3 | Off | `dfaa8acc` | 7 choices, 8 rows; 2px original inter-choice gap |
| `dialogue-layout-on/` | Russian | 1920x1440 | 3 | On | `dfaa8acc` | 7 choices, 8 rows; 4px inter-choice gap; 13 runs, 422 glyphs |
| `compact-inventory-off/` | Russian | 1920x1440 | 3 | Off | `eb2d3c4e` | Original-pixel control; byte-identical to `inventory-weight-off/` |
| `compact-inventory-on/` | Russian | 1920x1440 | 3 | On | `eb2d3c4e` | All 34 font-101 runs use 1.22 SemiBold; 218 glyphs |
| `compact-pipboy-off/` | Russian | 1920x1440 | 3 | Off | `eb2d3c4e` | Original-pixel control |
| `compact-pipboy-on/` | Russian | 1920x1440 | 3 | On | `eb2d3c4e` | Both font-101 runs use 1.22 SemiBold; 12 glyphs |
| `dialogue-layout-refactor-off/` | Russian | 1920x1440 | 3 | Off | `fe289ad6` | Full frame is pixel-identical to `dialogue-layout-off/` |
| `dialogue-layout-refactor-on/` | Russian | 1920x1440 | 3 | On | `fe289ad6` | Dialogue-window crop matches `dialogue-layout-on/`; full frame has 135 differing pixels outside the window |

The dialogue helper keeps every option accepted by the original fit estimate and also limits gaps by the real no-draw wrap end. Runtime geometry logs confirm all seven options remain visible, all eight text rows fit, and the button rectangles do not overlap. The dialogue OFF image is byte-identical to the earlier accepted dialogue OFF frame. The existing fixture sends KEY_2, observes a changed reply, and the engine maps keycode 50 to option index 1; this validates the keyboard-selection path, not a simulated mouse click.

The `CompactUi` size role is attached to text runs and follows copied records. SharpText renders those runs at 1.22 while untagged font-101 runs remain at the selected global 1.29. Main inventory slot labels, summary, weight, and selected-item descriptions use the role; loot and trade remain at 1.29. PipBoy text uses the same role, including its direct title, pagination, holiday, and health draws. The inventory OFF frame is byte-identical to the earlier `inventory-weight-off/` frame. Compared with the older task 056 capture, its only difference is the documented 432-pixel lower-HUD rectangle outside the inventory window.

The follow-up dialogue smoke after moving layout orchestration into `scene::dialogue_layout` preserves all seven options and eight rows. OFF remains at the original 2px gap; ON remains at 4px. The full OFF frame is pixel-identical to the prior accepted capture, and the dialogue-window pixels in the ON frame are pixel-identical to the prior accepted ON capture. The ON full frame has 135 differing pixels in physical rectangle `(588,375)..(1181,626)`, outside the dialogue window; no cause is asserted.

- Inventory: `runs/057-interface-font-size-preview/accepted/final/inventory-off/scr00103.png` vs `runs/056-interface-text-colors/accepted-b99d12c3-20261024/ru-x3-inventory-off/scr00087.png`; earlier exact frame: `runs/056-interface-text-colors/accepted-20261024/ru-x3-inventory-off/scr00079.png`.
- Dialogue: `runs/057-interface-font-size-preview/accepted/final/dialogue-off/scr00105.png` vs `runs/056-interface-text-colors/accepted-b99d12c3-20261024/ru-x3-dialog-off/scr00088.png`.
- Character: `runs/057-interface-font-size-preview/accepted/final/character-off/scr00101.png` vs `runs/056-interface-text-colors/accepted-b99d12c3-20261024/ru-x3-character-off/scr00089.png`.

All Russian x3 character fixture logs report font-101 line height `10..10` logical pixels. The corrected character plaque captures use the scoped two-pixel modern overlay lift. Dialogue and character OFF frames are pixel-identical to their latest task 056 references; the inventory window is unchanged and the documented lower HUD exception lies outside its text.
