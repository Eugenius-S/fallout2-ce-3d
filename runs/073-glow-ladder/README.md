# 073: final glow calibration

Owner's final choice: one global glow-strength slider, default 2 for barrel
and campfire. Internally compensate the brighter white/cyan computer glow;
no per-kind controls. Default 2 is level 3 in the five-step comparison.

Columns in each kind.jpg: OFF, then levels 1..5 at 0.5, 1, 2, 4, 8.
The near/wide halo weights remain 0.55/1.5. The shader reduces only cool or
white emission according to Rec.709 luminance, bounded to gain 0.5..1.
Pure warm fire and green slime keep gain 1. No new emission marks or objects.

The earlier dark monitor/inert slime samples were INVALID: the test disabled
cycling and then restored _cmap, whose cycling entries are unmapped zeros.
snapshot_palette.h now supplies cycle.cc's real rest-phase colors. Both
existing sources glow correctly with that fixture. This changes the test
preparation, not the game's palette or emission masks during normal play.
The earlier snapshots/measurements in Git history are superseded by these.

All views: 1920x1080, scale 2, ambient 16384, hour 0. Every six-frame row
comes from one frozen process with identical camera, hero, frame and palette.
All captures omit the HUD. Crops are identical across levels, no exposure edits.
JPEG evidence follows AGENTS.md; full raw PNGs remain untracked under build/.

| Kind | Map / elevation | View | Hero | Zoom |
| --- | --- | --- | --- | --- |
| barrel | klamall / 0 | 21700 | 21900 | 2 |
| campfire | kladwtwn / 0 | 22958 | 22963 | 2 |
| monitor | vault15 / 0 | 16492 | 16692 | 2 |
| screens | vault15 / 0 | 13275 | 13675 | 1 |
| slime | klatoxcv / 1 | 22924 | 19291 | 1 |

The slime crop includes the complete selected pool and surrounding floor/wall,
with margin for the halo. The screen is shown in its room, not the old solo view.
No energy-bolt fixture is available in the current harness; that conditional
sample was omitted.

Outcome figures:
- default-fires.jpg: both fires at the selected common default 2.
- compensation.jpg: same computer at strength 2, before/after compensation.
- screens-detail.jpg and slime-detail.jpg: OFF / default 2 / maximum 8.
- Five individual ladders plus fires.jpg/computers.jpg retain all five levels.

The same-palette control captures are in build/glow073/rest (barrel, campfire,
monitor) and rest2 (slime). compensation-check.json records the glow's added
RGB sum at strength 2 against OFF: monitor ratio 0.4957, campfire 0.9968,
barrel 0.9302, slime 1.0000. The barrel crop includes other cool/white spill,
so its total halo decreases slightly even though warm red source gain is 1.
OFF crops are pixel-identical before/after compensation in every control.

Reproduce each row with the table's parameters, for example:

```powershell
tools/dev-snapshot.ps1 -Map klamall -ViewTile 21700 -HeroTile 21900 -GlowLadder -Ambient 16384 -Hour 0 -Zoom 2 -CaptureConsole -OutputDirectory build/glow073/selected/barrel
tools/dev-snapshot.ps1 -Map vault15 -ViewTile 13275 -HeroTile 13675 -GlowLadder -Ambient 16384 -Hour 0 -Zoom 1 -CaptureConsole -OutputDirectory build/glow073/selected/screens
tools/dev-snapshot.ps1 -Map klatoxcv -Elevation 1 -ViewTile 22924 -HeroTile 19291 -GlowLadder -Ambient 16384 -Hour 0 -Zoom 1 -CaptureConsole -OutputDirectory build/glow073/selected/slime
py -3 tools/glow_ladder_sheets.py
```

measurements.json records raw PNG/executable hashes, crop coordinates,
strength metadata, and per-level means. The tool validates six states and
unchanged scene metadata. Source commit metadata may precede the final commit;
the recorded executable hashes identify the actual working builds.

Validation: Release game/mapper build, 15 CTests, 64 exporter tests, 12 regression
unit tests. All 16 ordinary glow-off views match independent current-base
controls from 066 exactly (0 pixels). Older committed references are unchanged;
their known drift is tracked in 058. Owner game/config were not restarted/installed.
