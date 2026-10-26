# P1 dev tools: launch and instructions

Date: 2026-10-18. Branch `scene-renderer`: P0 `49be0b4` + dev tools (this
commit). Specification: edition 2, the section "Mandatory dev mode for the
executor".

## The exact launch command

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File 'E:\Projects\fallout2-ce-3d\tools\dev-launch.ps1' -Map kladwtwn -Elevation 0
```

The launcher itself sets in `game\fallout2.cfg` a 16:9 window as tall as the
monitor above the task bar and the ×2 scale (`scale=2`); `-Aspect 21:9`,
the full monitor width, `-FullScreen`: the whole screen without a frame
(`windowed=2`). Ctrl+F11 in the game restarts it in the other window format
(16:9 ↔ 21:9): the engine sets the screen size once at start. Also
`[ui] skip_opening_movies=1`.
Then it starts straight into the 3D scene:

```
game\fallout2-ce.exe --dev-tools --dev-new-game --dev-map=kladwtwn --dev-elevation=0 --dev-walk-preview
```

`-Classic` keeps the usual 2D picture; Ctrl+F10 leaves 3D in the game.
Before the launch the launcher puts a fresh build from `build\` into
`game\`; if the game is open, its exe is renamed and it keeps running.

A background check without a window on screen: `tools\dev-snapshot.ps1`
runs the build as `build\test-run\fallout2-ce-test.exe` with the fixed
config from `tools\test-config\fallout2.cfg` (the owner's `game\fallout2.cfg`
is never read or written), off screen, without taking focus, takes one
snapshot and closes only its own process. With `-Probe`
(`--dev-pick-probe`) the game cursor is put on the scene residents, and
`game\logs\devprobe.log` records what the game finds under it; then a click on
the floor (the hero walks) and a click on the nearest person (a dialog).
Two snapshots: the hex cursor and the action icon.

In 3D the mouse is the game's: hover, action icons, the right button changes
the mode, a click walks or acts; the bottom bar and messages are visible over
the scene.

The 3D camera follows the game's screen center: scrolling at the window
edges, the arrows and following the hero move it; tilt and distance do not
change. It does not jump across hexes but glides smoothly to the center the
game picked (a time constant of 80 ms, immediately on a map change);
character animations are as in the game. In 3D mode scrolling is not limited
by the Restoration Project map borders and the scroll stoppers: they are
computed for the width of the 2D window, and on a wide window a narrow map
left no sideways move. The rule "no farther from the hero" holds, stretched
to the window the same way 480 × 400 related to 640 × 480.

- Profile: since 2026-10-18 a new game with the first ready-made character
  by default (`--dev-new-game`, without the elder movie): the Restoration
  Project requires a new game. `-Slot N` loads save N from
  `game\data\SAVEGAME` instead. The slots in `game\` are the dev sandbox;
  user saves are not touched.
- `--dev-map`/`--dev-elevation` are optional; without them you stay on the
  save's map. A wrong name/floor is refused; you stay on the save's map with
  no half-loaded scene (verified with `--dev-map=zzzz`).

## CLI

- `--dev-tools`: enables the mode (the panel, the hotkeys).
- `--dev-new-game`: start a new game right away with the first ready-made
  character.
- `--dev-map=<name>` / `--dev-elevation=<0..2>`: the target after the save
  loads or the new game starts.
- `--dev-list-maps`: prints all maps from the game resources (155 on the
  current DATs) and exits. Log: `runs/p1-listmaps.log`.

## The panel (Alt+D opens/closes)

- filter: a live case-insensitive filter over map names (arrows,
  selection, Backspace, printable characters);
- elev: [0][1][2]: floor choice; the current one is marked, `-`: the floor
  is absent on the current map;
- `<prev` / `next>`: the neighboring map of the list; `GO`: travel;
  `SNAP`: a snapshot;
- the current map/floor and the active renderer are shown (`scene-gl`: for
  now the GL presentation of vanilla; light: off until P2).

Hotkeys: Alt+D: the panel, F9: a snapshot (works with the panel open too).

## The fly camera

Ctrl+F12 in the 3D scene takes off: a free camera for looking at a map up
close and recording videos. The mouse turns it, WASD moves it, Q / E go down
and up, Shift is faster and Ctrl slower. While it flies it owns the keyboard
and the mouse and shows the world alone (no interface, no egg); the game keeps
running. Ctrl+F12 or Esc lands it and puts the game's view back; it lands by
itself when the 3D scene stops showing (combat, dialog). F9 takes a snapshot
from the air. `dev-snapshot.ps1 -Fly` (`--dev-fly`) takes one from where the
camera takes off; `-FlyTurn D -FlyPitch D -FlyDistance U` (`--dev-fly-turn=`,
`--dev-fly-pitch=`, `--dev-fly-distance=`) first circle it round the point it
looks at: D degrees round, D degrees above the floor, U units from it.
A map tour stop may end in `:solo:turn:distance`
(`kladwtwn:0:15123:1:90:3.5`): the things within `solo` hexes of the view
tile alone, from the fly camera `turn` degrees round and `distance` units
away; the shapes eval shoots a whole map in one run this way.
The fly tour (`py -3 tools/tour/tour.py --before SIDE [--after SIDE]`, task
078) shoots every spot of `tools/tour/spots.json` from the fly camera at
-45, 0 and +45 degrees, one run per map elevation and side, frozen (`-Still`)
in full daylight, and writes `before | after` sheets to `build/tour/sheets/`
(`changed.jpg`, then `changed-2.jpg`..., holds only the spots that changed,
or says none did). A side is a built checkout
or a scene cache folder; a cache side runs this checkout's build with
`dev-snapshot.ps1 -SceneCache DIR`.
`-Turn N` (`--dev-camera-turn=N`) takes the snapshot with the 3D camera N
quarter turns round (legacy snapshot units). `-TurnDegrees D`
(`--dev-camera-degrees=D`) accepts an exact angle, e.g. -45 or 45. Both
snapshot overrides bypass player settings; the dev fly camera is unlimited.
For players, `[3d] camera_rotation` enables [ and ] in 45-degree steps,
clamped to -45..+45. `[3d] camera_rotation_unlimited` (off by default,
marked not recommended in the Camera menu) permits a full turn. Turning rotation
off returns to the game view; turning unlimited off clamps any current turn
back into the safe range.
`-Solo R` (`--dev-solo=TILE,R`, TILE is `-ViewTile`) draws only the things
within R hexes of it, without walls or roofs (the hero stays: the fly camera needs it): one thing seen alone
from every side (the shapes eval, `tools/shapes/eval/eval.py`).

From the air things are seen from any side: sprite cards stand upright and
face the camera, and solids show their far sides (the near sides' pixels
mirrored, as the donor's first person does); the game's camera culls those.
Doors and gates do not turn: they lie on their wall's plane. Cards no more
than 20 pixels tall lie flat on the floor.
Walls show their backs and the strips that close their ends: `walls.bin`
(version 4) keeps them in batches only the fly camera draws.

## Transitions

Only the stock mechanism: `mapSetTransition` → the load in the safe phase of
the game loop (`mapLoadById`), the hero on the map's entrance tile, the
camera centered, the map scripts run. In combat/dialog/movies the
transition is refused with a status in the panel.

## A snapshot

F9/SNAP writes `devshot-YYYYMMDD-HHMMSS.json` + `scrNNNNN.png` into `game\shots\dev\<date>\` and `game\shots\game\` (the game folder root keeps only the game's own files):
the map, floor, renderer, light, internal resolution, drawable, CE commit,
SHA-256 of the running EXE, time.

## Acceptance (the spec, "Acceptance of the dev tools"): done

| Step | Result | Evidence |
|---|---|---|
| Open Klamat in one launch | PASS | `runs/p1-dev-klamat.png` ("entered Klamath", the hero at the entrance) |
| Travel to another map | PASS | `runs/p1-dev-den.png` (denres1), `runs/p1-den2.png` (argarden) |
| Floor | fixed, a manual check is still due | choice/refusal/fact are separated (see below) |
| Return to Klamat | PASS | `runs/p1-back-wtwn.png` (KLADWTWN through the stock .SAV mechanism) |
| Hero/camera/picking | PASS | the hero at the entrance, the camera centered; a click → the hex cursor: `runs/p1-pick-click.png` |
| Refusal of a nonexistent name | PASS | `--dev-map=zzzz` → stays on the save's map: `runs/p1-badmap-refused.png`; an empty filter + GO = no-op: `runs/p1-dev-back-klamat.png` |
| A snapshot with metadata | PASS | `game/shots/dev/2026-10-18/devshot-20261018-111259.json` (map/elev/renderer/light/res/commit/sha256; the 2026-10-18 capture sat in the root, the folder layout came later) |

## Limits

- The floors of the target map are not checked before the load (the map data
  becomes visible after the load; the elev row shows the actual floors of
  the current map). A hop between floors of multi-floor maps is in P1.
- Movies are not presented under the GL presentation (skip_opening_movies=1
  in the dev profile).
- Dev transitions between maps are not proof of quest transitions or of
  campaign playability (spec §6).

## Review of 2026-10-18 (an external check of a650e85) and fixes (ce 6dda27c)

1. The floor was not applied: tile=-1 → the engine skips setting the floor.
   Now a same-map switch passes enteringTile; cross-map: a follow-up
   correction after the load (update()).
2. The panel/snapshot showed the panel's choice, not the actual floor. Now
   the engine's actual gElevation; the panel choice is a separate value
   "(want eN)".
3. Floor presence was determined by _square != null (never null). Now the
   MAP_HEADER_ELEVATION_* bits from gMapHeader.flags: as in the loader; a
   same-map transition to an absent floor is refused at once.
4. The floor click zone is aligned with the labels.
5. A list click accounts for the scroll (mirroring the draw).
The snapshot metadata gained: camera (the center tile) and tools_commit
(the launcher passes --dev-tools-commit).

Floor check status: the fixes are built; the absence marks are confirmed on
KLADWTWN ([0]-[1]-[2]-, runs/p1e-vault-filter.png); the floor-to-floor
transition itself was not automated in the game because of input degradation
after window minimizations (I will try to check by hand: Alt+D →
filter arcaves → ENTER → RIGHT → ENTER, e1 in the panel).

## Second review (6dda27c) and fixes (the current build, 11:55)

1. Inverted floor presence: CE stores a SET bit as the ABSENCE of floor data
   (the loader reads tiles when (flags & bit) == 0). The check is inverted,
   as it must be. Confirmed in the game: KLADWTWN is now [0] [1]- [2]-
   (runs/p1e2-refused.png).
2. The floor of the target map is checked BEFORE the transition: devtools
   reads the map header (.SAV preferred, as mapLoadByName) from the
   resources and refuses without leaving the current map. The follow-up
   correction is kept as a backstop.
3. The list row height is captured at draw time (font 101) and used by the
   click handler.

Status: the refusal branch and the new floor computation are built; the
automated run of the transition to an existing floor hits the instability of
the injected input (ENTER arrives, arrows/BACKSPACE are eaten by the scroll
layer, panel clicks after minimize/restore are unstable). A manual minute:
Alt+D → want e1 by clicking [1] → GO → the status "elevation 1 absent on
KLADWTWN" with no transition; on arcaves ([1] present) → GO → actual e1 in
the panel/snapshot.

## Third review and closing the dev mode (2026-10-18)

1. An unreadable target map header (-1) now REFUSES the transition
   ("cannot read map header") instead of silently allowing it.
2. The pending floor correction moved from the draw path into the main game
   loop (main.cc, after mapHandleTransition()).
3. The follow-up transition uses the hero's current tile instead of
   enteringTile (a number of maps have enteringTile = 0/-1 and the engine
   skips placement).
4. In the panel the floor is picked with keys 1/2/3 (the arrows are eaten by
   the map scroll layer).

Floor acceptance (a successful run, 2026-10-18 12:14):
- a refusal BEFORE the transition: KLADWTWN + want e1 → GO → "elevation 1
  absent on ARDEAD"/"…on KLADWTWN", the map did not change
  (runs/p1f-refusal.png);
- a successful transition: KLADWTWN → arcaves → "2" → GO → the follow-up in
  the game loop → actual e1 (runs/p1f-arcaves.png, runs/p1f-e1-done.png);
- a snapshot with the full metadata: game/shots/dev/2026-10-18/devshot-20261018-121404.json +
  game/shots/game/scr00001.png (the 2026-10-18 capture sat in the root) (ARCAVES.MAP, elevation 1, camera, renderer, light,
  resolutions, ce_commit 01d73df, tools_commit cbeffec, exe SHA-256).

Dev mode status: closed per the reviewer's conditions (a successful
floor-to-floor transition + a refusal on an absent one + a JSON with
metadata).

## Tile choice on a floor change: fixed, acceptance of the current build awaits a check

A tile of the target floor is picked for the hero at a distance of at most 4
from his current position. A candidate is accepted only if the tile number is
greater than zero and in bounds, `squareTileFromTile` returns a valid square,
the floor frame `FloorTileFrmId` is not `TileFrameId::Grid`, and
`_obj_blocking_at` and `_obj_scroll_blocking_at` find no blocking objects.
The distance is checked by the engine's `tileDistanceBetween`; the search
walks full rings by growing distance. If they give no candidate, the map's
entrance tile goes through the same checks. With no safe candidate the
transition is refused without repeated corrections.

The criterion conservatively discards empty floors and known blockers. A
drawn floor by itself does not prove walkability or reachability of the
tile; the algorithm does not claim a path connectivity check.

Walking screenshots after the hop on ARCAVES e1 (`runs/p1g-e1-before.png`,
`p1g-e1-walk2.png`, `p1g-walk-a.png`, `p1g-walk-b.png`) belong to the
previous version of the tile check. They do not confirm the behavior of the
current build after the floor, scroll-blocker and bounds checks. Acceptance
of the safe tile choice in the game for the current build awaits a separate
check.

`runs/p1-devtools-map-header-regression.ps1`: a forensic check of the
offsets and fields of a real `KLADWTWN.SAV`; it is not a unit test of the
probe and not a game acceptance.

## Restarting the owner's game

Only when the owner asks. The game in `game/` reads `game/mod-cache`, and the
engine and the exporter change together, so build and export together, in the
main checkout, on `main`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release
py -3 tools/scene_export/export.py --game-root ./game --maps klamath
powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-launch.ps1 -Slot 1
```

`dev-launch.ps1` puts the new build in place of the running one and starts it
from save slot 1; the owner closes the old window. Never export into
`game/mod-cache` without that build: an older engine cannot read a newer
export.

## Glow comparisons

`dev-snapshot.ps1 -GlowStrength X` (`--dev-glow-strength=X`) implies `-Glow`
and accepts 0..8. Values use an invariant decimal point. The player menu's
slider covers the same range; the owner-selected default is 2.

`-GlowLadder` captures glow off, then 0.5, 1, 2, 4, 8 in one frozen scene.
It implies `-Still -CyclePalette`, rejects other fixtures/glow overrides,
and suppresses the HUD in all six captures. Each JSON records `glow.enabled`
and `glow.strength`. It does not change the owner's executable or config.

`-CyclePalette` (`--dev-cycle-palette`) restores cycle.cc's rest-phase
cycling entries 229..254 before the still view loads its object textures.
Ordinary snapshots and normal gameplay keep their original palette behavior.

The rest phase is explicit: color.pal leaves cycling entries unmapped, so
_cmap contains zeros there. snapshot_palette.h supplies the engine's real
6-bit cycle colors. Snapshot palette changes are opt-in and do not alter
normal gameplay. One global glow-strength menu slider controls all sources;
white/cyan emission is luminance-compensated in the shader, with gain 0.5..1.
Warm red fire and green slime retain their existing contribution.
