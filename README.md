# Fallout 2 CE 3D

https://github.com/user-attachments/assets/e2fb04bc-243a-49a0-a775-7aa46c1ee8d2

This project gives Fallout 2 a volumetric 3D world around its original sprites.
Turn-based combat, dialogs, quests, saves and the rules stay the same. The view
keeps Fallout's oblique angle and atmosphere; this is still the same game, not
a shooter.

## Install

[Download the launcher](https://github.com/Eugenius-S/fallout2-ce-3d/releases/download/latest/fallout2-ce-3d-launcher.exe)
from the rolling [latest release](https://github.com/Eugenius-S/fallout2-ce-3d/releases/tag/latest).

1. Install Fallout 2 from Steam or GOG.
2. Run the launcher. It detects those installs; if it cannot find the game,
   choose its folder containing `master.dat` and `critter.dat`.
3. The first install and later updates need an internet connection.

The launcher installs or updates `fallout2-ce.exe`, `ce.dat` and
`scene-exporter.exe`, then builds scene data from your game files. An existing
`fallout2.cfg` is kept; a default is installed only when it is missing. The
launcher does not replace the original `fallout2.exe` or alter save files. The
game may add missing `[3d]` defaults when it saves its settings.

## What works

- Floors, walls, roofs and large objects form a 3D scene. Characters remain
  the original sprites and move as they do in Fallout 2.
- Sun and moon lighting and shadows follow game time. Lamps and other point
  lights add local light and glow.
- The perspective camera follows the game smoothly and can turn. Mouse-wheel
  zoom eases to its new level. The scene works at any screen resolution.
- The in-game `3D OPTIONS` panel includes `Camera zoom` and
  `Shadow edge softness` for softer shadow borders. A developer fly camera is
  available for scene inspection.
- Scene data is currently generated for Klamath areas. Where scene data is
  unavailable, the game uses its original 2D view.

## Switch to 2D

The in-game `3D OPTIONS` panel and the `[3d]` section of `fallout2.cfg` control
the scene. Press `Ctrl+F10` to return to the original view. To start with 3D
off, set:

```ini
[3d]
enabled=0
```

## Build from source

You need CMake, Ninja, Visual Studio's C++ workload, Python 3.12+ and your
Fallout 2 game files. The build script uses its local MSVC and Windows SDK
when available; otherwise it detects a Visual Studio installation with
`vswhere`.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release
ctest --test-dir build --output-on-failure
py -3 -m pip install -r tools/scene_export/requirements.txt
py -3 -m unittest discover -s tools/scene_export -p 'test*.py' -v
py -3 tools/scene_export/export.py --game-root ./game --maps klamath
```

The game files and generated scene data stay local. See [game setup](docs/game-setup.md),
the [technical plan](docs/2026-10-17-fallout2-ce-3d-env-p0-p1.md) and the
[3D design](docs/2026-10-17-fallout2-sprite-lighting-design.md).

## Thanks

- **Interplay and Black Isle**: for Fallout 2.
- **Alexander Batalov** ([fallout2-ce](https://github.com/alexbatalov/fallout2-ce)):
  for Fallout 2 Community Edition, without which this project would not exist.
- **The [fallout2-ce](https://github.com/fallout2-ce/fallout2-ce) community**:
  for continuing to develop the engine with fixes, sfall and mod support.
- **[Apolotary](https://github.com/Apolotary/fallout2-first-person)**:
  for fallout2-first-person. The approach to restoring map volume and part
  of the converter come from there.

## License

The engine and converter code are distributed under the terms of the original
projects: Sustainable Use License 1.0 (see `ce/LICENSE.md` and the licenses
in `tools/scene_export/`). This is an unofficial non-commercial fan project.
