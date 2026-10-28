# 091: launcher and the rolling latest release

Evidence for tasks/091-launcher-updates.md. The launcher was exercised on a
fresh copy of the game files at `<game>`
(`master.dat`, `critter.dat`, `data/` copied in; the owner's `game/` was
never written to), against a fake release served on 127.0.0.1:8119 holding
the same payload CI publishes: `fallout2-ce.exe`, `ce.dat`,
`fallout2.cfg`, `version.txt`, `scene-exporter.exe`.

| File | Scenario |
| --- | --- |
| run1-install.log, run1-3d-kladwtwn.png | Fresh install: download, unpack, files placed (cfg as the default), 7 Klamath maps exported by the frozen exe, 3D scene on kladwtwn. |
| run2-update.log, run2-update-3d-kladwtwn.png | Update a091 -> b091: "fallout2.cfg: kept (the player's file is never overwritten)", re-export, game runs in 3D. |
| run4-update-c091.log | Update to the fixed binaries with --no-start. |
| run5-offline.log, run5-offline-3d-kladwtwn.png | Release unreachable: "offline: starting the installed game as it is", exporter --check reports "scenes present", no re-export, 3D scene. |

Every run ends with the launcher hashing `fallout2.cfg` and the whole
`data/savegame` tree before and after the game: unchanged in all logs.

Notes:

- The shot of each run is the dev one-shot snapshot the launcher's game
  arguments produced (kladwtwn, tile 25076), `scene_3d_enabled: true` in the
  companion devshot json of the same test copy.
- The game exits 0xC0000005 when closed through WM_CLOSE (the player's X) in
  this dev-snapshot flow; the launcher logs the code and still reports the
  hashes.
- On first run the game itself creates `ddraw.ini`, `debug.log`, `logs/` and
  `mods/mods_order.txt` beside it; the launcher never touches those.
