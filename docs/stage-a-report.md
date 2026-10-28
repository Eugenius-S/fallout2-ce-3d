# Stage A report: the game base (fallout2-ce on a copy of the clean game)

Date: 2026-10-17. Specification:
`<game>\docs\superpowers\specs\2026-10-17-fallout2-sprite-lighting-design.md`.

## Pinned base

- Upstream: https://github.com/fallout2-ce/fallout2-ce, commit
  `fb8ba51e016b09597bc0b4277fbac26899e43342` (2026-10-17, "Typesafe sound.h
  enum usage (#868)"): see `pinned-commit.md`. The clone lives in `ce/` and
  is not part of this repository. No changes to the CE sources.
- Build: `tools/configure-build.ps1` (Ninja + MSVC 14.40, SDK 22621,
  CMake 4.4). All targets built: `fallout2-ce.exe`, `mapper-ce.exe`,
  `ce-dat-tool.exe`, `ce-frm2png.exe`.

### Toolchain quirks of this machine

1. VS BuildTools has a broken vcvars (vswhere does not see VC.Tools): the
   environment (PATH/INCLUDE/LIB) is set by the script manually.
2. CE defines `_STATIC_CPPLIB` (a static STL). With CMake's default dynamic
   runtime (`/MD`) linking fails with LNK2038/LNK1319 (libcpmt.lib = MT
   against MD objects). The fix without touching upstream:
   `CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>`: the
   whole build is static (/MT), the exe is self-contained.

## Working copy of the game

- Source: `<game>` (read-only; the
  reference was not modified; CE was never run in it).
- `game/`: `master.dat`, `critter.dat`, `patch000.dat`, `fallout2.cfg`.
  Checksums (md5): `runs/a-dat-checksums.txt`:
  - master.dat `5acf4f9a8bb017af0f7b35c7f920a48b`
  - critter.dat `9b16e92ac88d168e81126b72e6f9ab3c`
  - patch000.dat `6afa3fde17b95191cb6bae18bfd10291`
- Important: CE reads/writes `fallout2.cfg` and creates `data/` **next to
  its exe**, not in the current directory. The correct layout is the exe in
  the game folder (`game/fallout2-ce.exe`), as in a stock CE install. A test
  run of the exe from `build/` left a `build/fallout2.cfg` with defaults,
  this explains why `skip_opening_movies=1` in `game/fallout2.cfg` did not
  work at first.
- `game/fallout2.cfg`: the `[ui]` section got `skip_opening_movies=1`
  (skipping IPLOGO/INTRO/CREDITS during checks; the logic is
  `src/main.cc:96`).

## What was verified

| Check | Result | Evidence |
|---|---|---|
| Building the pinned CE without changes | PASS | 594/594 ninja targets |
| Running our exe on the working copy | PASS | `runs/a-launch*.log`, screenshots |
| Decoding MVE movies and slides | PASS | `runs/a-afterclick*.png` |
| Splash → menu → new game (Temple of Trials), HUD and world drawn | PASS | `runs/a-state4.png` |
| Saving (SLOT01: SAVE.DAT, ce.sav, sfallgv.sav) and loading the save in our build | PASS | `game/data/SAVEGAME/`, user session 15:20, fullscreen |

The "map transition, combat, dialog, inventory" checks of the stage A gate
are interactive: partially automated, the final confirmation belongs to a
manual session on the working copy (the user already plays our build). The
save load in the user's session effectively covers "map loading" too.

## CE differences from the original (the pinned version)

From the upstream README (+SFALL_COMPATIBILITY.md for sfall scripts):

- The resolution is not limited to 640x480 (`[screen] resolution_x/y`,
  `windowed`).
- Party members can loot and barter for the PC; direct party equipping.
- Inventory/loot in 2 columns, barter in 4 rows, an extended AP bar.
- Ctrl-click to move items quickly; highlighting of
  items/corpses/containers/critters (sfall-mods.ini).
- Music plays between maps; 44.1 kHz stereo sound, .ogg/.wav besides .acm.
  Note: without OGG files nothing changes for a clean game.
- Auto-opening doors, a built-in HELP menu, ESC cancels the elevator floor
  choice, the last save slot is remembered, a fancy ammo meter.
- "Dozens of small things": better pathfinding, fewer graphics glitches.
- The save format is extended (a save gains `ce.sav`, `sfallgv.sav`); test
  saves of the original exe only on copies (done: all work in `game/`, the
  reference untouched).

In short: CE does not promise the exact behavior of the original (as the
specification requires); the differences are listed above and grow as they
are verified.

## Open dependencies

- Russifier (stage B): the package is not chosen by the user. Russian
  support is NOT claimed. After the choice: pin the source/version/sums and
  a reproducible install instruction on a copy.
- 60 FPS (stage C/D): not measured, not claimed.

## Reproduction

```powershell
cd <worktree>
powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release
Copy-Item build\fallout2-ce.exe game\
# run: game\fallout2-ce.exe
```
