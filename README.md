# Fallout 2 CE 3D

This is an attempt to bring Fallout 2 into 3D while keeping it the same
Fallout 2. Turn-based combat, dialogs, quests, saves and the rules do not
change. Characters stay as the original sprites, and a volumetric world
appears around them: walls, buildings, large objects, a camera with a light
perspective and live light.

Why. First, so the game runs on modern hardware and large monitors without
ritual dances. Second, so the picture improves but keeps its atmosphere:
lamps and campfires light the scene for real, buildings cast sharp shadows,
characters darken in shadow and take the color of the nearest light, and the
sun moves across the sky with game time.

We are not making a shooter out of Fallout; there will be no free camera and
no new character models.

## What already works

A prototype on one Klamath area (the `kladwtwn` map, first elevation):

- floor and walls are assembled from the game's resources;
- the hero walks over the volumetric scene; animation and direction come
  from the game;
- the camera looks at the same angle and scale as the game itself, with a
  light perspective; the map scrolls together with the game;
- a click on the ground picks a hex; the engine builds the path;
- residents, animals, furniture, items and doors come straight from the game
  in their current state: they move and change as in the original;
- a dev mode for development: pick any map and elevation right in the game.

Next on the plan: picking objects with the mouse, the game interface over
the scene, roofs, then light and shadows. After that the other maps and an
installer.

Details: [the specification](docs/2026-10-17-fallout2-sprite-lighting-design.md)
and [the technical plan with TODO](docs/2026-10-17-fallout2-ce-3d-env-p0-p1.md).

## How the repository is laid out

- `ce/`: the engine: a fork of Fallout 2 CE with the new renderer.
- `tools/scene_export/`: the converter of maps from the game's resources
  into a 3D scene.
- `tools/`: build and run scripts.
- `docs/`, `runs/`: plans, reports and check screenshots.
- `game/` and `build/`: the local copy of the game and build results; they
  do not go into Git.

Game resources, saves and built EXEs are not put into the repository. You
need your own legally purchased Fallout 2. We play with the Restoration
Project and a Russian translation; preparing the game is described in
[game-setup.md](docs/game-setup.md). The reference Steam install is used
read-only; the game runs from the copy in `game/`.

Old experiments live in the `archive/*` branches: DLSS5 over the original
game, a D3D12 viewer and the first version of the perspective camera.

## Building

You need CMake, Ninja, MSVC, Python 3.12+ and the Fallout 2 files. The build
script has the paths to MSVC and the Windows SDK of this machine baked in;
on another computer you will have to adjust them.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release
ctest --test-dir build --output-on-failure
py -3 -m pip install -r tools/scene_export/requirements.txt
py -3 -m unittest discover -s tools/scene_export -p 'test*.py' -v
py -3 tools/scene_export/export.py --game-root ./game
```

`game/fallout2.cfg` must point to the game's DAT files. The dev launch is
described in [p1-devtools.md](docs/p1-devtools.md).

Fresh changes from upstream CE are pulled like this:

```powershell
git fetch upstream
git merge -X subtree=ce upstream/main
```

## Thanks

- **Interplay and Black Isle**: for Fallout 2.
- **Alexander Batalov** ([alexbatalov/fallout2-ce](https://github.com/alexbatalov/fallout2-ce)), for Fallout 2 Community Edition, without which this project would not
  exist.
- **The [fallout2-ce](https://github.com/fallout2-ce/fallout2-ce) community**, for continuing to develop the engine: fixes, sfall and mod support. Our
  engine is a fork of their version.
- **[Apolotary](https://github.com/Apolotary/fallout2-first-person)**:
  for fallout2-first-person. The approach to restoring map volume and part
  of the converter come from there.

## License

The engine and converter code is distributed under the terms of the original
projects: Sustainable Use License 1.0 (see `ce/LICENSE.md` and the licenses
in `tools/scene_export/`). This is an unofficial non-commercial fan project.
