# Fallout 2: CE with a volumetric environment and the original sprites

Date: 2026-10-17. Edition 2. The current specification for the executing
agent.

**The user's decision:** keep the game and its mechanics in CE, use the
reconstruction of the environment by the principles of
Apolotary/fallout2-first-person. First a live area of Klamath; the other
maps: after the integration is confirmed.

This edition replaces the former direction of "perspective cardboard + light"
in this file. The former plan of the perspective cardboards and the
native-dlss5 documents of 2026-10-12 are historical. The current technical
plan P0–P1: `2026-10-17-fallout2-ce-3d-env-p0-p1.md` in this folder.

## 1. The goal and the boundaries

The same Fallout 2: turn-based combat, quests, dialogs, inventory, saves,
atmosphere. The image: the original textures and sprites, a volumetric
environment, a fixed perspective camera from above at an angle, then dynamic
light.

The priority: a full game → correct geometry and controls → light →
performance → the other maps → installation. DLSS5 stays an optional late
goal.

- Windows x64. The base of the game (a user decision of 2026-10-18): the
  original Fallout 2 from Steam + the Restoration Project (RPU) v2.4.34 by
  BGforge with a Russian translation. The source install
  `<game>` is read-only.
- The working project `<worktree>`; running on the copy in
  game/.
- The Russian language is the translation from the RPU release
  (`rpu_russian.dat`, `upu_russian_sound.dat`). The repack `<game>`
  is not the base; later only the look of the bottom bar is taken from it.
- Characters stay sprites. Simple geometry of walls, furniture, roofs and
  landmarks is in scope.
- Full new character models, a shooter, free camera rotation, changing the
  rules and a neural repaint are out of scope.
- Do not build a viewer of all maps first and then rewrite the game in it.

## 2. The base and the actual state

| Part | Pin | Purpose |
|---|---|---|
| CE upstream | fb8ba51e016b09597bc0b4277fbac26899e43342 | The source of the game rules and state |
| Current CE C1 | e14218e0a3486ca94f548d2fcc7d846b6352f77a | Keep as an experiment, do not develop as the accepted architecture |
| The donor Apolotary | 0da1cd449f1dc7f2f6986ff7e8728a4b6ae20584 | The converter and the sample of spatial rendering |
| The donor path | tools\scene_export\vendor (a copy of the donor) | Read and move the chosen parts keeping the notices |

The donor has a verified export of 12 featured maps, not the whole game. The
converter accepts other maps; the quality and coverage need checking. Walls
and some objects are reconstructed approximately; unknown objects stay
sprites.

Before changes record the HEAD and the dirty state. Since 2026-10-18 the
engine ce/ and the documents/tools live in one repository
`<worktree>` (GitHub Eugenius-S/fallout2-ce-3d); one commit
describes everything. Do not overwrite someone else's changes. The old
experiments (DLSS5, the native prototype, C1) live in the archive/* branches.

The reports docs/stage-a-report.md and docs/c1-camera-prototype.md hold
limited checks. Do not declare all the mechanics accepted based on a launch
and a save load. Keep the list of CE's differences from the original EXE.

## 3. Architecture

```text
INPUT: the resources of the installed game + the live CE state + the camera settings.
TRANSFORM: a local converter restores the surfaces, textures and bindings.
TRANSFORM: the CE adapter passes the current objects, doors, roofs and animation frames.
TRANSFORM: the GPU draws the geometry and the sprites with one camera, then the original UI.
MEANING: the reconstruction changes the image; the rules and the state stay in CE.
OUTPUT: a playable Fallout 2 with a volumetric environment and perspective.
DIRECT CHECK: one save and the same actions in the stock and the new renderers.
```

### CE: the only owner of the game

Do not rewrite combat, scripts, AI, pathing, inventory, saves and the game
light. The renderer reads the state; the mouse pick is converted into
ordinary CE actions. The geometry does not create an independent world of
collisions: walkability and interactions are decided by the engine.

### The converter: the representation of the environment

Adapt the existing Python converter; do not write DAT/MAP/FRM parsers anew
without an established need. Study tools/fp/scene.py, proj.py, the wall/item
modules, viewer/geom.js and render.js.

| Content | Representation |
|---|---|
| Floors, roofs | Surfaces with textures converted from the original isometry |
| Recognized walls | Planes of the right direction, thickness, the top face |
| Doors | Separate elements driven by the CE state and animation |
| Furniture and decorations | Simple shapes by the donor's rules: boxes, cylinders and so on |
| Characters | The original directed sprite and the current CE frame |
| Unknown objects | A visible fallback: a correctly anchored sprite/tilted plane |
| Flat objects | A decal, if the classification is confirmed |

Keep the hex, the elevation, the anchor points and the FRM offsets. The
height of a picture is not the height of an object: the bottom may depict
depth/ground. The universal C1 approach "all objects are vertical, the
height = frameHeight" is forbidden.

Hidden sides are an approximation, not a restored truth. Do not delete
unknown objects for a pretty frame. For every map count the geometric
objects, the fallbacks and the errors; the exceptions must be visible in the
report.

### Live objects and the cache

A static export is not a save. Doors, characters, items, containers and the
elements changed by scripts are updated from CE. After an opening/deletion a
baked copy must not remain.

The minimal adapter contract (design data, not a claim of a ready API):

| Data | Requirement |
|---|---|
| Map | An identifier, the elevation, the load generation, the coordinate conversion |
| Object | A runtime handle with a generation; a binding to the source MAP object, if one exists |
| Position | A hex, the elevation, the movement/animation offset, the anchor point |
| Look | PID/FID or an equivalent, the direction, the frame, the FRM offsets, the palette/alpha, visibility |
| Changes | Creation/deletion, movement, a look change, a door, roof hiding |
| Picking | A match of the visible surface/sprite to a CE object |

Equal PIDs do not mean one object. A memory address is not a stable key.
After a load/map change recreate the runtime bindings. Abandoned and
script-created objects work without a pre-existing export record.

On a small area a per-frame sync of the list is acceptable. Take the state in
an agreed phase of the game loop; do not read mutable pointers
asynchronously.

The cache holds the format/converter version, the fingerprints of the input
resources and the parameters. On a data change it is rebuilt. A corruption
gives a diagnostic and a return to the stock view. Follow the actual resource
order of CE, including loose files: the donor's export of three DATs does not
guarantee a match to a localized install.

### The native renderer

One application and one game window of CE. The donor's browser is a visual
reference, not a second game engine.

The preferred backend of the first prototype: OpenGL 3.3 through SDL2, with
an explicit adaptation of the WebGL2 algorithms/shaders. First verify the
GPU context, the original UI, resize and the return to the stock output.
That is not a verified integration yet. If such a path conflicts with CE,
describe the concrete reason and update the technical plan before moving the
scene.

A depth buffer, alpha masks, a consistent occlusion of surfaces and sprites
are needed. Keep the palette, cycling, fades, transparency and highlighting.
A one-time conversion to RGB must not freeze the palette animation.

The UI, menus, text, dialogs and video are not subjected to
perspective/lighting. The stock renderer is kept as the control and the
fallback. For the technical choice of the backend a restart is allowed at
first; the acceptance comparison of the stock/new view must switch on a live
scene without resetting the game.

## 4. The camera and the controls

- A real perspective from above at an angle, a fixed direction, pan, limited
  zoom. The view angle agrees with the drawn sides of the sprites.
- Show two variants of the perspective strength on one save with a
  comparable central coverage. Record the FOV/pitch/distance/zoom.
- The geometry, the objects, the shadows and the picking use one coordinate
  system. Account for the elevation, the viewport, the DPI/window size. Do
  not treat the vanilla screen coordinates as ready world coordinates.
- The ground is picked by a ray onto the plane of the current floor. An
  object: by its visible representation and handle; a ground projection is
  not enough for a door/a tall wall.
- Transparent pixels do not intercept the pick. The cursor/highlight match
  the image in the center and at the edges; the UI handles its clicks first.
- Keep the roof hiding, the visibility behind a wall and the special
  transparency of CE. The old draw order does not prove the correctness of
  the new perspective.
- Do not mask the lost tiles with a vignette. Distinguish a real map edge
  from a clipping error; limit the pan to the available scene.
- The stock/new view and the light OFF/ON switch independently. The temporal
  history resets on a view switch, if one appears.
- The character's direction and frame come from CE. Do not stretch the
  sprite and do not speed up the game animation for FPS.

## 5. Light and authenticity

The first showing: without the new light. After the geometry is accepted
add one movable point source: different sides of a wall, the floor and an
item react to the direction/distance. Then bind it to an existing lamp/fire.

The geometry uses its own normals. For characters/fallbacks a careful
simplified light is allowed at first. Normal/height maps: only for a
verified set; one map for all the character poses is forbidden. The
brightness of the original picture does not prove the shape of a surface.

Account for the already drawn shadows. Keep the palette/texture; do not
replace volume with a global gloss/bloom. New shadows: after the light is
accepted, with a check of an open/closed door and a roof.

The new light does not affect sneak, hits, visibility and the other CE
rules. Light OFF removes the new effect but does not have to make the
reconstructed scene pixel-equal to the original. The stock renderer exists
for that comparison.

## 6. Stages and acceptance

### P0: the technical plan and the output

Research the actual integration points of src/svga.cc, tile.cc, object.cc,
art.cc, game_mouse.cc, map.cc, palette.cc. The new plan holds concrete
files/functions, the state contract, the coordinates, the cache, the
dependencies and the check commands. Do not present assumed functions as
existing ones.

Keep C1. Verify the GPU + the original UI inside CE, a stock launch, resize
and the return to the old view. A primitive is acceptable for a short
context check but does not count as a visual result.

### P1: a live Klamath area

#### The mandatory dev mode for the executor (added 2026-10-18)

Before the next visual checks implement the tools for researching maps
inside the game CE. The mapper has a sample of the map list
(`src/mapper/map_func.cc`, `map_load_dialog`), but running the editor does
not replace checking the mechanics in the game. The current
`--dev-load-game=N` loads only a save; a ready map-choice CLI in the verified
`src/main.cc` does not exist. The config `[debug] mode=environment` does not
enable such a mode by itself.

- The dev launch is always windowed (`[screen] windowed=1` in the working
  copy), without the intro and a manual character creation. A separate
  launcher uses a fixed test profile/copy of a save, defaulting to Klamath
  `kladwtwn`, elevation 0. The regular launch keeps its purpose.
- Add an explicit `--dev-tools` and in that mode `--dev-map=kladwtwn`,
  `--dev-elevation=0`, `--dev-list-maps`. These are required new options, not
  existing commands. Build the list from the available game resources, not
  from the donor's 12 featured maps; check the available floors by the map
  data.
- In the game itself: a compact panel with a search/choice of a map, a choice
  of the available elevation and previous/next map buttons. Provide keyboard
  access without a conflict with the ordinary actions. The current
  map/elevation and the active renderer are visible in the panel.
- Do the load by the stock CE transition in a safe phase of the loop. Do not
  copy the `mapLoadByName` call from the mapper into an arbitrary handler:
  finish the old state, recreate the renderer bindings, restore the
  cursor/UI. During combat, a dialog, a video or a load the transition is
  refused with a clear message or deferred to a safe state.
- After the switch the hero stands on a valid entrance tile of the
  corresponding floor, the camera is centered. A name/floor error leaves no
  half-loaded scene. The usual map scripts keep working: the dev teleport
  does not disable them for a pretty picture.
- The vanilla/new renderer and light OFF/ON switches are independent. If the
  new renderer of a map is not ready, the panel explicitly reports a
  fallback; the stock picture must not be presented as the reconstruction.
- A snapshot button/hotkey saves the image and the metadata: map/elevation,
  camera, renderer, light, the internal resolution, the CE/tools commit and
  the EXE SHA-256. Take the metadata from the actual state, not from the
  requested launch parameters.
- The dev session uses separate test saves/changeable map data; it does not
  overwrite the user slots and the resources of the original game. Moving
  between maps by debugging is not proof of the stock quest transitions or of
  campaign playability.

The acceptance of the dev tools: in one launch open Klamath, travel to
another available map, pick an existing floor, return to Klamath; check the
hero/camera/picking, the absence of old objects, the refusal of a
nonexistent name/floor, save a snapshot with metadata. Record the exact
launcher command and a short instruction for the next agent. Do not declare
the panel ready based on enabling debug logs or windowed mode.

The map kladwtwn, elevation 0. Export enough of the surroundings for the
area and the camera movement. Pick an area with a wall, a door, an
item/container and an NPC. Before acceptance record the concrete objects,
the save, the camera and the action sequence. Do not replace Klamath with
the temple or a synthetic scene without justification.

| Check | PASS condition |
|---|---|
| View | Spatial walls, characters on the ground without flattening, no doubled/missing objects; two camera variants shown |
| Walking | Clicks in the center and at the edges match the hexes; the path/obstacles are decided by CE |
| Door | Opening/closing consistently changes the image and the walkability; no old copy |
| Item | Pick up, drop, pick up again; the image matches the inventory |
| Talk | A real dialog with an NPC from the new view, a working UI |
| Combat | Target choice, movement, an attack, ending the turn and the opponent's turn; AP/animations from CE |
| Save/load | Save the changed door/items, restart and load; everything restored |
| Transition | Leave for another map and return with no old objects/handles; an unsupported map explicitly uses the stock renderer |
| Modes | Switching the view without resetting the game; highlighting, roofs and the UI work |

Evidence: a short continuous recording of actions, paired frames of one
state, the error log, both commits, the SHA-256 of the running EXE. A
screenshot or an export does not replace the game checks.

**Before the mass conversion the user must accept the look of P1.** If the
picture does not fit, fix it on one area.

### P2: light and performance

Show light OFF/ON at one camera, moving a source around a wall/item.
Surfaces react; the UI and the rules are unchanged. Check the animations,
transparency and the conflict with the drawn shadows.

The goal: 60 FPS at 1920×1080 on the user's RTX 5070. Record the CPU/GPU/
driver, the settings and the internal resolution. The same route: 15 seconds
of warm-up + 60 seconds of measurement separately for the stock view, the
new one without light and the new one with light. Keep the median/p95 CPU/GPU
frametimes and the frame interval. The criterion: the median and p95 of the
frame interval ≤16.7 ms with no hidden resolution reduction; measure the
loads separately. Note missing GPU metrics; do not conclude the cost of
light from a single FPS.

Game time and the animation rate are not sped up. 60 FPS of output do not
mean 60 unique source frames of a character.

### P3: all maps

After P1/P2 convert the remaining maps. Build the list from the game's
resources/registry, including the used elevations, interiors and random
encounters; the 12 featured maps are not enough.

A machine report: map/elevation → export → CE load → geometry/fallback →
visual check → game check → errors. Do not mix a successful export with a
confirmed playability. A repeated run uses only a valid cache with matching
inputs.

To claim "all maps are converted": the full list, no exceptions of the
stock renderer and a visual check of every map/elevation pair. "The game is
playable" requires separate checks of the routes/quests. The behavior matrix
includes a street, a tight interior, a cave, an elevator/floors, a random
encounter, roofs, translucent walls and script creation/deletion.

### P4: the Russian language and installation

The russifier is chosen: the RPU v2.4.34 translation (the source and hashes
are in `docs/game-setup.md`). The resources are in place; the look in the
game is not verified yet. Check the menus, dialogs, the journal, the line
breaks, the fonts, the voice-over if present. Do not promise EXE patches and
logic changes as ordinary resources.

The installation uses the existing game and creates an isolated mod/cache
directory. It does not overwrite the original EXEs, DATs, configs, saves.
Check the install, the update, the removal and the launch of the original.
Keep the CE saves separate; test the compatibility with the original EXE
only on copies.

### P5: the optional DLSS5

A separate research after an accepted useful game. Choosing OpenGL does not
guarantee a future compatibility with the NVIDIA runtime. The base mod
without a neural DLL; do not include the experimental kit from the old
project into a release automatically.

## 7. Structure, verification and reproducibility

Keep ce/, build/, game/, tools/, docs/, runs/. The proposed areas:
ce/src/scene_renderer/: the GPU and the adapter; tools/scene_export/: the
converter; game/mod-cache/: the derived data. These new paths are not
claimed as implemented yet; clarify them in the technical plan.

Follow the CE/donor style; do not reformat the whole upstream. Separate the
coordinates, the adapter and the output. Meaningful automated checks: the
conversion/inverse projection at different elevations, equal PIDs,
creation/deletion, reset/load, cache invalidation. A formal test of a
formula does not replace the visual and game acceptance.

The existing PowerShell commands:

```powershell
git -C '<worktree>' status --short
git -C '<worktree>' rev-parse HEAD
powershell -NoProfile -ExecutionPolicy Bypass -File '<worktree>\tools\configure-build.ps1' Release
Get-FileHash -Algorithm SHA256 '<game>\fallout2-ce.exe'
Start-Process -FilePath '<game>\fallout2-ce.exe' -WorkingDirectory '<game>'
```

The build script may recreate build on a generator change: read first, keep
the needed artifacts. A build does not prove the game/EXE was updated: check
the result/copy path and the hashes, record the install command. Add new
test/export commands after creating the tools; do not invent ready commands.

## 8. Distribution and safety

A free non-commercial mod with available sources. Keep the LICENSE/NOTICE of
both upstreams, note the borrowings/modifications, do not re-license someone
else's code as MIT/GPL. Before a release check the terms of the pinned
dependencies.

DATs, extracted art, music, movies, saves, the russifier and the derived
scenes do not go into Git/the public package. The conversion is local, from
the user's install. The published items are the permitted code and tools,
with the unofficial status stated; that is not a guarantee of the absence of
claims.

Do not touch the dirty configs of the old install and someone else's work.
Local commits are small; the CE sources are kept separately from the external
documents. Since 2026-10-18 the user ordered a GitHub: the accepted stages
are committed and published in the project's private repositories; releases
stay a separate task.

## 9. The handoff to the executor

> Work by edition 2. Do not continue C1/C2 and do not rewrite the game in a
> viewer. Keep the existing work, research the combination of the Apolotary
> converter with the live CE and compose a new technical plan P0–P1 with the
> exact files/contracts/checks. Then the minimal GPU output inside CE and a
> kladwtwn area: volumetric walls, sprite characters, a door, an item, a
> talk, combat, a save. The first visual showing without the new light. Mass
> export: after the result is accepted. Report separately what is built,
> what is visible, what is verified by actions and what is not verified yet.

Sources: https://github.com/fallout2-ce/fallout2-ce ;
https://github.com/Apolotary/fallout2-first-person ; docs/how-it-works.md and
the README / Verification status of the donor. Their integration is not
counted as done by this specification.
