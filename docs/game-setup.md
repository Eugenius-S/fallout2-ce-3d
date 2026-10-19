# Preparing the game in `game/`

Since 2026-10-18 the game for development is the original Fallout 2 from
Steam plus the Restoration Project (RPU) by BGforge with a Russian
translation.

## Source

- RPU v2.4.34, https://github.com/BGforgeNet/Fallout2_Restoration_Project/releases/tag/v2.4.34
- `rpu_v2.4.34.zip` — SHA-256 `b8f8a9c900a553c4e781e74bcf805f5663bc37b3633ebfbebfc928b70470285e`
- `rpu_v2.4.34_translations.zip` — SHA-256 `cc9ca1e12e477cbf4cd0156097d4b9185f370033e5ac8ce9c4ef16f741b45644`
- CE's README declares full Restoration Project support.

## Steps

1. Copy the Steam install into `game/` by copying, not hard links: RP moves
   files, and the Steam install must stay untouched.
2. Put the built `fallout2-ce.exe` and the project's `fallout2.cfg` into
   `game/`.
3. Unpack `rpu_v2.4.34.zip` into `game/` with overwrite and run
   `bash ./rpu-install.sh` from `game/`. The script moves `patch000.dat`,
   the old `data/` and `f2_res.dat` into `backup/rpu/`.
4. From the translations archive copy `rpu_russian.dat` and
   `upu_russian_sound.dat` into `game/mods/`. Both are already listed in
   `mods/mods_order.txt`.
5. In `game/fallout2.cfg` set `language=russian` and
   `f2_res_dat=mods\f2_res.dat`. The dev launch sets up the screen itself:
   a window across the monitor, every game pixel twice as large.
6. Rebuild the scene: `py -3 tools/scene_export/export.py --game-root ./game`.

RP requires a new game: saves from the original do not fit. The dev launch
starts a new game with the first ready-made character by default
(`--dev-new-game`), see [p1-devtools.md](p1-devtools.md).

A copy of the game without RP lives in `game-vanilla/` (also outside Git)
for comparison.
