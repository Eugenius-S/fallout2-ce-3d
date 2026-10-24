# Font 101 size preview

All captures show the real Russian character screen at 1920x1440, logical 640x480, scale 3.

| Variant | Full PNG |
| --- | --- |
| 1.00 | [character.png](preview-1x00/character.png) |
| 1.08 | [character.png](preview-1x08/character.png) |
| 1.15 | [character.png](preview-1x15/character.png) |
| 1.22 | [character.png](preview-1x22/character.png) |
| 1.29 | [character.png](preview-1x29/character.png) |
| 1.29 Bold 700 | [character.png](preview-1x29-bold/character.png) |
| 1.29 SemiBold 600 | [character.png](preview-1x29-semibold/character.png) |

`normal-control/character.png` is the same run without the preview option. The normal control and explicit 1.00 image have identical decoded RGBA pixels. Root also independently verified that the 1.00 control matches task 056's normal character capture. Relative to the normal control, 1.08 changes 136,330 pixels, 1.15 changes 151,815, 1.22 changes 173,025, and 1.29 changes 189,934.

The full PNG, snapshot JSON, and successful fixture stdout are in each variant folder. Variants 1.00, 1.08, and 1.15 use source `95700dbd`, SHA-256 `08ed3bd52afffb2cf5de5a5e59df914805d06a63897a7b9a75f8350eaa227382`. Variants 1.22 and 1.29 use source `d25c69e6`, SHA-256 `5d4fb2aca955bb8addad0582e793eb677e053e5904f050a0ccf7f4a32b0f25f1`. Each group shares one executable and fixture state. The D/E source commit only adds the two preview scales and their tests. The stdout confirms preview scale and visible font-101 overlay records.

CTest passed 14/14 on source `d25c69e6`. Renderer tests compare the captured mask against unrestricted drawing at x2 and x3 for И, Й, Е, Ё, ё, Ѓ, and `g`; they check that glyph pixels stay inside the requested text width and preview coverage. For a 9-row line, preview coverage is 9 rows at 1.00, 10 at 1.08, 11 at 1.15 and 1.22, and 12 at 1.29.

Visual review of the actual screens found no visible neighboring-row collisions or clipped diacritics, including 1.22 and 1.29. These are preview variants only. The production default remains 1.00 pending the owner's choice.

## E face comparison

The 1.29 Bold 700 and SemiBold 600 captures show the same real character screen, game state, 1920x1440 dimensions, logical 640x480 resolution, and scale 3. Both use source `c0bf3b08` and EXE SHA-256 `d2b51753f3d393339e913c4bba0766c84d915bd2d3a08aa63f5819594dec15c6`. Their decoded RGBA images differ at 97,233 pixels. The E-size renderer test confirms the SemiBold CP1251 `Ё` stays inside the captured preview mask, and that it renders differently from Bold. CTest passed 14/14 on this source.

SemiBold is available only through the opt-in character fixture. Font 101 keeps Bold 700 by default, and no other font uses the override. Visual review found no visible clipping or neighboring-row collision in the SemiBold frame. The production face remains Bold 700 pending the owner's choice.
