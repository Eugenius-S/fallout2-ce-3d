# Font 101 size preview

All captures show the real Russian character screen at 1920x1440, logical 640x480, scale 3.

| Variant | Full PNG |
| --- | --- |
| 1.00 | [character.png](preview-1x00/character.png) |
| 1.08 | [character.png](preview-1x08/character.png) |
| 1.15 | [character.png](preview-1x15/character.png) |
| 1.22 | [character.png](preview-1x22/character.png) |
| 1.29 | [character.png](preview-1x29/character.png) |

`normal-control/character.png` is the same run without the preview option. The normal control and explicit 1.00 image have identical decoded RGBA pixels. Root also independently verified that the 1.00 control matches task 056's normal character capture. Relative to the normal control, 1.08 changes 136,330 pixels, 1.15 changes 151,815, 1.22 changes 173,025, and 1.29 changes 189,934.

The full PNG, snapshot JSON, and successful fixture stdout are in each variant folder. Variants 1.00, 1.08, and 1.15 use source `95700dbd`, SHA-256 `08ed3bd52afffb2cf5de5a5e59df914805d06a63897a7b9a75f8350eaa227382`. Variants 1.22 and 1.29 use source `d25c69e6`, SHA-256 `5d4fb2aca955bb8addad0582e793eb677e053e5904f050a0ccf7f4a32b0f25f1`. Each group shares one executable and fixture state. The D/E source commit only adds the two preview scales and their tests. The stdout confirms preview scale and visible font-101 overlay records.

CTest passed 14/14 on source `d25c69e6`. Renderer tests compare the captured mask against unrestricted drawing at x2 and x3 for И, Й, Е, Ё, ё, Ѓ, and `g`; they check that glyph pixels stay inside the requested text width and preview coverage. For a 9-row line, preview coverage is 9 rows at 1.00, 10 at 1.08, 11 at 1.15 and 1.22, and 12 at 1.29.

Visual review of the actual screens found no visible neighboring-row collisions or clipped diacritics, including 1.22 and 1.29. These are preview variants only. The production default remains 1.00 pending the owner's choice.
