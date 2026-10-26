# 077: player camera limit

`kladwtwn-angles.jpg`: -45, 0 and +45 degrees, left to right. Captured on
kladwtwn, elevation 0, hero tile 17665, view tile 17467, still frames,
1920x1080 at scale 2. Each frame is reduced to 640x360 for the sheet.
`captures.json` records the raw PNG and executable hashes. The reported
source commit is the claim commit; captures used the working implementation.

`options-russian.jpg`: Camera section of the real preferences panel, Russian,
sharp text, 1920x2160 at scale 2, cropped to (320,1680)-(1600,2080).
Both toggles are off by default. The unrestricted option is marked not recommended.
Final menu crop uses `build/camera077/menu-ux` after the owner's copy revision.

Reproduce:

```powershell
tools/dev-snapshot.ps1 -Map kladwtwn -HeroTile 17665 -ViewTile 17467 -Still -TurnDegrees -45
tools/dev-snapshot.ps1 -Map kladwtwn -HeroTile 17665 -ViewTile 17467 -Still
tools/dev-snapshot.ps1 -Map kladwtwn -HeroTile 17665 -ViewTile 17467 -Still -TurnDegrees 45
tools/dev-snapshot.ps1 -Preferences -PreferencesPosition Bottom -ScreenWidth 1920 -ScreenHeight 2160 -Scale 2 -Language russian -SharpText
```

Release build and all 15 CTests pass, including endpoint/reversal, full turn,
revoking unlimited rotation, real menu registry controls, and RU/EN metadata
coverage. Exporter unit tests: 61 pass. Regression unit tests: 12 pass.
The keyboard animation itself was not manually exercised in the owner's game.
Background snapshots intentionally bypass the player's settings; they show
the endpoint rendering, while executable tests check target angle limits.

World regression uses the independently captured current-base controls from
066, the same scene cache and toolchain. Committed older references remain
unchanged; their existing drift is tracked in 058. The compact numeric result
is in `regression-summary.json`.
