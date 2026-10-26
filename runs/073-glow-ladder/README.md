# 073: glow strength choices

Columns: OFF, then levels 1..5 at glow_strength 0.5, 1, 2, 4, 8.
Current default 1 is level 2. Owner approved barrel strength 1, working computer 2, campfire 4.
These need per-kind calibration; the current global default is unchanged.
Near/wide weights remain 0.55/1.5. No new emission marks or objects were added.

All rows: 1920x1080, scale 2, zoom 2, ambient 16384, hour 0. Every level in a
row comes from one frozen process with identical camera, hero and frames.
The fixture restores color.pal's authored cycling entries and omits the HUD
at every level. Identical crops, 360x300 cells; no exposure adjustments.
JPEG sheets follow AGENTS.md; full PNGs remain untracked under build/.

| Kind | Map / elevation | View | Hero | Notes |
| --- | --- | --- | --- | --- |
| barrel | klamall / 0 | 21700 | 21900 | atorch4, the existing 050 fire-barrel fixture |
| campfire | kladwtwn / 0 | 22958 | 22963 | woodfire, embers in its still frame |
| monitor | vault15 / 0 | 16492 | 16692 | comp9, working blue computer |
| slime | klatoxcv / 1 | 19091 | 19291 | green pool crop, almost no response |
| screens | vault15 / 0 | 13275 | spawn | monitr7, solo radius 1, dark screen |

Limitation: the selected pool and wall screen develop no visible halo even
at 8. Only 6 and 4 pixels change by more than 8/255 against OFF, respectively;
barrel 13435, campfire 2562, working computer 18245. These numbers do not prove
a cause. Source monitr7 has blue pixels; the captured screen remains black.
Do not treat the almost identical columns as a successful glow comparison.
A source/occlusion/mask investigation is recorded in inbox. No energy-bolt
fixture exists in the current harness, so that conditional sample was omitted.

Reproduce using the table's parameters, for example:

```powershell
tools/dev-snapshot.ps1 -Map klamall -ViewTile 21700 -HeroTile 21900 -GlowLadder -Ambient 16384 -Hour 0 -Zoom 2 -CaptureConsole -OutputDirectory build/glow073/final/barrel
tools/dev-snapshot.ps1 -Map vault15 -ViewTile 13275 -Solo 1 -GlowLadder -Ambient 16384 -Hour 0 -Zoom 2 -CaptureConsole -OutputDirectory build/glow073/clean/screens
py -3 tools/glow_ladder_sheets.py
```

measurements.json records raw image hashes, crops, strength metadata and
per-level means. The generator checks exactly six captures, the expected
strengths and unchanged scene metadata. Checks: Release build, 15 CTests,
64 exporter tests, 12 regression unit tests. No owner game restart/install.

All 16 ordinary views match current-base controls from 066 exactly (0 pixels).
Old committed references remain unchanged; their drift is tracked in 058.

Owner confirmed the lack of glow on the sampled wall screen and slime.
