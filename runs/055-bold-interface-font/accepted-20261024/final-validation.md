# Task 055 visual and test evidence

Each capture directory contains the PNG, snapshot JSON, and fixture console
log. JSON sidecars record the exact source commit, executable SHA-256,
resolution, logical resolution, scale, and sharp-text state.

| Screen | Text mode | Source | Executable SHA-256 | Evidence |
| --- | --- | --- | --- | --- |
| Inventory | Original pixels, sharp off | `1ddaa05f` | `2694a4fa06c6fe9b8fbceff2b4b3f45687f7f0000230b9c90d97cff1408e151d` | `ru-x3-inventory-original/` |
| Inventory | Bold 700, sharp on | `1ddaa05f` | `2694a4fa06c6fe9b8fbceff2b4b3f45687f7f0000230b9c90d97cff1408e151d` | `ru-x3-inventory-bold/` |
| Sulik dialogue | Original pixels, sharp off | `1ddaa05f` | `2694a4fa06c6fe9b8fbceff2b4b3f45687f7f0000230b9c90d97cff1408e151d` | `ru-x3-dialog-original/` |
| Sulik dialogue | Bold 700, sharp on | `1ddaa05f` | `2694a4fa06c6fe9b8fbceff2b4b3f45687f7f0000230b9c90d97cff1408e151d` | `ru-x3-dialog-bold/` |
| Character screen | Original pixels, sharp off | `1ddaa05f` | `2694a4fa06c6fe9b8fbceff2b4b3f45687f7f0000230b9c90d97cff1408e151d` | `ru-x3-character-original/` |
| Character screen | Bold 700, sharp on | `1ddaa05f` | `2694a4fa06c6fe9b8fbceff2b4b3f45687f7f0000230b9c90d97cff1408e151d` | `ru-x3-character-bold/` |

All six captures use Russian, a 1920x1440 physical framebuffer, a 640x480
logical framebuffer, scale 3, the same source commit, the same executable, and
the same private Klamath test setup. Inventory and character use their normal
engine keys. Dialogue uses the real Sulik conversation fixture. All three
screens have fixture PASS records; every Bold capture has an active drawn
sharp-text pass.

## Face and layout checks

- Renderer test verifies font 101 raster output equals font 103/104 Bold and
  differs from font 102 SemiBold.
- Existing CP1251 tests render `И`, `Й`, `Е`, and `Ё` through font 101 at line
  height 9 and verify line-mask clipping at scales 2 and 3.
- CTest passed 14/14, including `embedded_fonts_tests`.
- Scene exporter unit tests passed 44/44.
- Regression checker unit tests passed 12/12.
- Official sharp-off regression passed 16/16 with zero changed pixels. The
  copied summary is `off-regression-16/summary.json` and `summary.txt`; the full
  run is `runs/regression/20261024-184104-597-8092/`.

The Bold change only selects the existing embedded Bold 700 face for sharp
font 101. It does not change font size, the fitting or placement algorithm,
palette colors, fades, other face selection, or other font IDs.
