# Font 101 size preview

All captures show the real Russian character screen at 1920x1440, logical 640x480, scale 3. The preview-only fixture uses one Release executable built from source `95700dbd`, SHA-256 `08ed3bd52afffb2cf5de5a5e59df914805d06a63897a7b9a75f8350eaa227382`.

| Variant | Full PNG |
| --- | --- |
| 1.00 | [character.png](preview-1x00/character.png) |
| 1.08 | [character.png](preview-1x08/character.png) |
| 1.15 | [character.png](preview-1x15/character.png) |

`normal-control/character.png` is the same run without the preview option. The normal control and explicit 1.00 image have identical decoded RGBA pixels. Root also independently verified that the 1.00 control matches task 056's normal character capture. Relative to the normal control, 1.08 changes 136,330 pixels and 1.15 changes 151,815 pixels.

The full PNG, snapshot JSON, and successful fixture stdout are in each variant folder. All three variants use the same source revision and EXE SHA. The stdout confirms preview scale and visible font-101 overlay records.

CTest passed 14/14. Renderer tests compare the captured mask against unrestricted drawing at x2 and x3 for И, Й, Е, Ё, ё, Ѓ, and `g`; they check that glyph pixels stay inside the requested text width and preview coverage. Capture tests confirm that the normal line height is unchanged and preview coverage extends to 10 rows at 1.08 and 11 rows at 1.15.

Visual review of the actual screens found no visible neighboring-row collisions or clipped diacritics. These are preview variants only. The production default remains 1.00 pending the owner's choice.
