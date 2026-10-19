# Technical plan P0–P1: CE + a volumetric environment (edition 2)

Date: 2026-10-17. Specification: `specs/2026-10-17-fallout2-sprite-lighting-design.md`
(edition 2). Every function/file below was verified by reading the pinned
sources in the same session; assumptions are marked explicitly.

## 0. Pins and state

| Repository | State | Action |
|---|---|---|
| CE `E:\Projects\fallout2-sprite-lighting\ce` | `fb8ba51e` (master) + the experiment branch `sprite-lighting-c1` (`e14218e`, clean) | A new branch `scene-renderer` off `fb8ba51e`. C1 is not developed |
| Donor `E:\Projects\fallout2-native-dlss5\vendor\fallout2-first-person` | `0da1cd4`, dirty: `viewer/fp.js` (my `lens` parameter from G2, documented, harmless) | Read-only; ports keep the NOTICE |
| Project `E:\Projects\fallout2-sprite-lighting` | `1e766f5`, clean | Documents/tools/reports |
| Working game `game/` | DATs from Steam (md5 in `runs/a-dat-checksums.txt`), exe = the C1 build | After P0 rebuild from the `scene-renderer` branch, record the SHA-256 |

## 1. Verified CE integration points (commit fb8ba51e)

| Point | Where | Role in the plan |
|---|---|---|
| `renderPresent()` | `src/svga.cc:473` | The single frame output point: `SDL_UpdateTexture` → `SDL_RenderClear/Copy` → `movieRenderDirectOverlay()` → `SDL_RenderPresent`. The vanilla/scene output switches here |
| `gSdlSurface` (8-bit) → `gSdlTextureSurface` (RGBA) | `svga.cc:222–330`, `directDrawSetPalette*` | Palette→RGB runs **every frame** via `SDL_BlitSurface`: fades and palette cycling behave the same in any backend; "freezing" the cycling threatens only the static textures of geometry (see §5) |
| Window | `_GNW95_init_window` `svga.cc:161` | Already created with `SDL_WINDOW_OPENGL` and the hint `SDL_HINT_RENDER_DRIVER=opengl`; `SDL_RenderSetLogicalSize` (svga.cc:367) — the logical-to-window scale. We take our own GL context on the same window and take over the whole output (P0) |
| Movies | `movieRenderDirectOverlay()` `src/movie.cc:318` | A separate SDL_Texture over the frame. On a scene-mode map movies do not play — the overlay stays only in the vanilla mode |
| World: floor/roof | `tileRenderFloorsInRect`/`tileRenderRoofsInRect` `src/tile.cc:1511/1279`, called from `tileRefreshGame`/`tileRefreshMapper` | The scene-mode flag skips the vanilla world drawing (the 8-bit buffer then holds only the UI) |
| Objects | `_obj_render_pre_roof` / `_obj_render_post_roof` `src/object.cc:805/906` (the order — `_obj_render_table_init` `object.cc:3492`) | The sync point: the adapter walks the same object lists (see the contract, §3) |
| Sprites/frames | `artLock/artGetFrameData/artGetWidth/GetHeight`, `Art::xOffsets/yOffsets` (`src/art.h`) | The source of pixels and anchors of character cards |
| Ground picking | `tileFromScreenXY` `src/tile.cc:824`, calls in `game_mouse.cc` (cursor/click) | Replaced by ray → floor plane → hex (the donor's inverse projection, `proj.py`/`geom.js`) |
| Roof hiding | `gTileRoofIsVisible`, `tile_toggle_roof` `src/tile.cc` | The scene mode must respect this state |
| Game light | `src/light.cc` (`lightGetTileIntensity`, ambient) | Untouched; on P2 used as data for luminance |
| UI/world text | `src/interface.cc`, `src/text_object.cc`, cursors `src/game_mouse.cc` (`_obj_move` of the cursor objects) | In scene mode these elements move to an overlay pass (§4) |

## 2. Architecture and coordinates

```text
CE (the only owner of state) ──snapshot before the frame──> SceneAdapter
The converter (an adaptation of the donor's tools/fp) ──once per map──> game/mod-cache/scene/<map>/<elev>/
renderPresent(): [vanilla] buffer texture → screen   [scene] GL: geometry+sprites with one camera → the UI strip from the buffer on top
```

- The world coordinates are hex ones, as with the donor: the `east/north`
  axes on the ground, the z height in the donor's "vanilla pixels"
  (`proj.py`: groundToPx/pxToGround, wall heights from `hpx`). **Vanilla
  screen coordinates are not world coordinates** (requirement §4 of the
  spec; the C1 approach is not repeated). A C++ port of `proj` already
  exists as a sample (`fallout2-native-dlss5/src/projection.{h,cpp}`,
  verified against JS to 1e-9) — it moves into `ce/src/scene_renderer/`
  with a NOTICE.
- Camera: a fixed azimuth (the direction of the game camera),
  pitch/FOV/distance/zoom parameters in the config; two strength presets
  for acceptance. Pan at the edges/by click as in CE (the view center =
  `tileSetCenter`), limited zoom.
- Rendering: OpenGL 3.3 core. The donor's single vertex format
  (pos3, offset3, texel2, wall1, twin2, kind1; kinds 0–5) and its card depth
  semantics (rowPlane) move as they are; the WebGL2 shaders → GLSL 150 (a
  mechanical adaptation of in/out/texture()). The GL loader — vendored
  `glad` (single-file, MIT, notice). Character sprites — cards from the
  current CE frame; walls/roofs/furniture — from the export; occlusion —
  a shared depth buffer (replacing the rowPlane rule for walls, cards keep
  their own depth plane).
- The vanilla view stays: the same GL context draws the CE buffer as a
  single quad — presentationally identical to `SDL_RenderCopy` (the same
  nearest/linear scale). Switching views on a live scene — without
  recreating anything.

## 3. The adapter contract (a state snapshot, the phase — before `renderPresent`)

| Data | Source in CE (verified) | Notes |
|---|---|---|
| Map/elevation/generation | `Map`, `gElevation` (`src/map.cc`), the load counter is kept by the adapter | On a load/map change the bindings are recreated |
| The object list of the area | Walking the same lists as `_obj_render_pre_roof/post_roof` (the map's tile lists) | A small area: a full walk around the view center, no cache between frames |
| Position | `Object->tile`, `->elevation`, `->x/y` (the movement offset), the anchor from `Art::xOffsets/yOffsets` | The card anchor point — the bottom of the sprite on the hex |
| Look | `Object->fid` → `FrmId` (type/REP/frame/rotation), `->frame`, `->rotation` | The frame is taken per frame; an LRU cache of frame textures |
| Doors/script changes | The flags/state of the objects in the snapshot; a diff against the previous frame | An open door = a different FRM/geometry, no "baked copy" |
| Roof hiding/visibility | `gTileRoofIsVisible`, `Object->flags` (HIDDEN/TRANS_*) | Translucent walls — through the alpha channel of the scene |
| Object picking | A GPU ID buffer (render-to-texture, objectIndex in a channel) → `Object*` | Transparent pixels do not pick (an alpha test in the ID pass); a ground projection is not enough |

Equal PIDs are different objects: the snapshot key is the address of the
map list node within one frame's phase + a generation; after a load/transition
the bindings are rebuilt. Objects created by scripts without a record in the
export go into a fallback card.

## 4. The scene frame and the UI

1. CE draws the frame as always; in scene mode `tileRefreshGame` skips the
   floor/roof/objects (the 8-bit buffer then holds the interface bar,
   floating text, cursors).
2. `renderPresent()`: a GL clear → the scene (geometry from the cache +
   cards from the snapshot) → on top, a texture from the 8-bit buffer,
   **only the interface bar rectangle and known UI zones**; the cursor and
   text elements of the world (the cursor sprite, the hex cursor, the
   outlines, `text_object`) are drawn by the adapter as cards/decals per
   their CE state (the list in §1, the UI row). This is the most
   labor-intensive part of P1; element by element with a checklist.
3. Palette fades (entering/leaving combat, transitions) — a scalar from the
   current fade of `palette.cc` is applied to the scene (the current value
   is read, not changed). Palette cycling of the static floor textures: not
   critical on P1 (Klamath's acid/fire stay vanilla cards), for geometry —
   re-uploading the affected small textures, marked as a limitation.

## 5. The converter and the cache

- `tools/scene_export/` — an adaptation of the donor's `tools/fp_export.py` +
  `tools/fp/{scene,proj,walls,boxes,props,things,atlas,light}.py` to run from
  the mod repository against `game/` (the master files/patch DATs are
  accounted in CE's order: `master.dat` → `critter.dat` → `patch000.dat` →
  `data/`).
- The output: `game/mod-cache/scene/kladwtwn/0/scene.json` + textures (the
  donor's format, version + fingerprint of the inputs in `scene.json`; a
  mismatch — a rebuild; a broken cache — a diagnostic and a refusal with a
  clear error, no auto-rebuild).
- On P1 we export only kladwtwn elevation 0 with enough margin around the
  area; the donor's 12 featured maps are not the converter's limit, but full
  coverage is not a P1 goal (that is P3).

## 6. Dependencies

- `ce/third_party/glad` (or a generated glad2: GL 3.3 core, loader) — MIT,
  notice.
- Python: numpy, Pillow (the donor already needs them); no new heavy
  dependencies.
- Build: the same `tools/configure-build.ps1`; watch that the script does
  not wipe `build/` on a generator change (it does not now — the generator
  is pinned to Ninja).

## 7. The order of work and checks

### P0 (a context check, ~a week)
1. The `scene-renderer` branch off `fb8ba51e`; `ce/src/scene_renderer/`
   (the GL context, present, the quad path) + `glad` in third_party.
2. `renderPresent()` on GL: the vanilla buffer as a quad. Checks: a stock
   launch (menu/new game/save), the FPS text, fades, palette cycling
   (fires), movies (in the vanilla mode), a window resize;
   `[lighting] camera=0` is not needed — the vanilla view == "the old view".
3. The P0 criterion: output visually indistinguishable from SDL_Renderer;
   save behavior unchanged byte for byte (a save before/after — `ce.sav`
   compatible). Not a visual result but a working context — a primitive is
   acceptable.

### P1 (a live kladwtwn area)
The map `kladwtwn`, elevation 0; an area with a wall, a door, a
container/item and an NPC is fixed BEFORE acceptance (the object list, a
save, the action sequence — in `runs/p1-plan.md`). Acceptance — the table of
spec §6 P1: the view (two camera presets), walking (clicks center/edges), a
door, an item, a talk, combat, save/load, leaving the map and returning
(unsupported maps explicitly use the vanilla view), the modes (switching
without a reset). Evidence: a continuous recording of actions, paired frames
of one state, the adapter error log (geometry/fallback/error counters per
map), both commits, the EXE SHA-256.
Before mass conversion — the user accepts the look.

### Commands (actual ones; to be extended as the tools appear)

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File 'E:\Projects\fallout2-sprite-lighting\tools\configure-build.ps1' Release
Copy-Item 'E:\Projects\fallout2-sprite-lighting\build\fallout2-ce.exe' 'E:\Projects\fallout2-sprite-lighting\game\'
Get-FileHash -Algorithm SHA256 'E:\Projects\fallout2-sprite-lighting\game\fallout2-ce.exe'
python 'E:\Projects\fallout2-sprite-lighting\tools\scene_export\fp_export.py' --map kladwtwn --elevation 0   # appears in P1
& 'E:\Projects\fallout2-sprite-lighting\game\fallout2-ce.exe' --dev-load-game=1                              # the snapshot save of the area
```

## 8. Risks (open)

- SDL_Renderer → our own GL: if a conflict appears (SDL_Renderer already
  holds the window's GL context) — in P0 we create the context BEFORE
  createRenderer and do not create an SDL_Renderer at all (all its uses in
  svga.cc are listed above; there are three). The fallback plan:
  SDL_Renderer stays for vanilla, the scene goes through a GL→CPU→texture
  readback (slow, for debugging only).
- A mismatch between CE's resource order and the donor's export of the three
  DATs — verify by CE's file list (`dbRead`/`xfile` order), a test: the sha
  of the scene files from the donor's export vs our converter on one map.
- A tilted camera at map edges — the same "void" problem as in C1: pan is
  limited to the available scene (requirement §4 of the spec), we do not
  mask with vignettes.

## 9. Project TODO — the user's decisions of 2026-10-18

Deferred tasks: they are not implemented features and do not replace the
acceptance of the dev mode and the volumetric Klamat in P1. Light comes
after P1 is accepted.

- [ ] Bind the visual 3D sources to the existing game sources: take the
  position, radius, brightness and current on/off from the CE state; refine
  the height, color and type per object (a lamp, a torch, a campfire). The
  visual settings do not change the game visibility calculations and CE
  mechanics.
- [ ] Add sharp dynamic shadows from buildings, roofs and the other
  volumetric objects. A door opening, roof hiding and object changes must be
  accounted; define explicitly how a shadow behaves when a roof is hidden.
- [ ] Make an adjustable light pixelation of the shadows in the style of the
  original graphics. Check the stability of the pattern with the camera and
  the sun moving.
- [ ] Darken the character sprite as a whole if its anchor point falls into
  a building's shadow. Remove the contribution of the covered source while
  keeping the ambient light and the other lamps; a short transition at the
  boundary avoids flicker.
- [ ] Character sprites cast no shadows but react to light: brighter near a
  source, darker away from it, taking the sources' color (warm from a
  campfire, cold from neon and so on). Light is computed by the sprite's
  anchor point, no normal maps for poses.
- [ ] Move the sun by CE game time: direction, height and color, long
  shadows in the morning/evening, no sunlight at night. Account for
  interiors, waiting, loading a save and game time jumps.
- [ ] Take the bottom game bar from the build `E:\Games\Fallout 2`
  (Restoration Project + russification): there it is stretched to wide
  screens. First identify the concrete UI mod and its resources, then move
  the graphics and the layout of the elements onto the working CE features.
  Not in the current stage.
- [ ] For large recognizable objects use separate simple Blender models and
  GPT Image textures; convert the ordinary surroundings automatically.
  Check the silhouette and palette in the game camera, keep the sources.
  Generate the textures small or explicitly downscale them after generation
  to the detail density of the surroundings; a fixed size is not chosen yet.
  Do not bake strong shadows conflicting with the new light into them.
- [ ] Lay the groundwork for running on macOS (a decision of 2026-10-18;
  Windows has priority, the port itself is later). Do not tie new things to
  Windows without need. When porting: a compatibility flag of the OpenGL
  context for Mac, Mac versions of the dev tools (Ctrl+F11, a background
  run, the launch and snapshot scripts), building and signing the app, a
  check on a live Mac. OpenGL on Mac is deprecated; if it is removed — 3D
  through a layer over Metal.
- [ ] Sharp interface text at the real screen resolution instead of the
  pixel one (a decision of 2026-10-19, for later). Now the game's font
  No. 101 (`FONT1.AAF` in `master.dat`: the Pip-Boy, dialogs, inventory) is
  drawn into the game's frame and stretches with it. The text needs a
  separate layer at the window resolution, keeping the colors, the line
  positions and the Cyrillic of the localization.

Light acceptance: the same scene state with the light OFF/ON, the character
inside and outside a shadow, a lamp on/off, a door and a roof, noon/sunset/
night. Profile the sun and the local sources with shadows separately; the
limit of active shadow sources is chosen by measurements, with no FPS
promises before a check in the game.

## 10. The accepted camera — 2026-10-18

The user approved the camera of the first native preview of the Klamath
walls:
eye `(-3.95610, 11.00000, 21.11265)`, target
`(-11.95610, 1.00000, 9.11265)`, vertical FOV `50°`, near `0.1`, far `400`.
This is the base camera: when live objects are added keep the angle, the
perspective and the scale. A future follow-the-hero must move the eye and
the target together and not change the tilt and distance without a separate
decision. Evidence: `runs/p1-wall-review-20261018/`. Only the static view of
the floor, walls and doors was accepted; the full P1 acceptance is still
ahead.

Later the user separately asked to narrow the FOV to `25°` and fix the
visible direction of the hero's card. For the wall/hero/walk preview the
eye-target offset is multiplied by `tan(25°)/tan(12.5°)` with the same
target, to keep the scale at the target; near/far stay `0.1/400`. After the
conversion of the donor's `(u,v,z)` into GL `(u,z,v)` the horizontal
screen-right uses `-cross(forward, up)`; the picker and the card apply the
same sign. The original FRM UVs and the CE rotation are not mirrored or
reassigned. The floor-only overview and its basis are kept. The earlier
recorded images and runtime reports refer to the old FOV of 50°.

## 11. Live objects in the scene — 2026-10-18

The residents, animals, furniture, items, doors and the other objects of
Klamath are drawn not from the export but from the engine's state in every
frame: the same animation frame, rotation and offset that
`_obj_render_object` uses. Standing objects are cards to the camera by the
hero card formula; flat ones (carpets, blood, corpses, exit zones) lie on
the floor under the pixels where the game draws them.

The walls and the panels in the wall planes are still baked into
`walls.bin`; the export writes their list into `baked.bin`, and the live
pass skips them. Doors are no longer baked: an open door is its current
frame in the engine.

Verified: the snapshot `runs/p1-objects-20261018/` (3506 objects, 0
skipped), the card math tests `ce/tests/sprite_card`. The orange hatching of
the exit zone exists in the original 2D picture too
(`runs/p1-dev-klamat.png`).

Not verified in the game: a door opening and the residents moving on
consecutive frames. The palette color cycles (fire, water) of the objects
still freeze: a frame is cached by fid/frame/rotation. The objects cannot be
picked with the mouse yet.

## 12. The game-like view — 2026-10-18

By the user's decision the objects and the camera moved to the rules of the
game view of the donor's viewer (`viewer/render.js`, `geom.js`):

- The projection is the projection of the game itself (an oblique parallel
  one; an ordinary camera cannot reproduce it) with a light perspective: in
  the plane through the screen center every point sits on the pixel of the
  2D game, at the same scale; the center is `gCenterTile`, as in the engine.
  A point's distance depends only on how much lower on the screen and higher
  above the floor it is: the screen axes of the game are not perpendicular
  on the floor, and the depth along the game's ray gave a skew (the right
  part of the frame farther than the left). The eye is 100 units (first 36.9
  from the 25° camera, then 52; the user twice asked to be closer to the
  game's orthography: the residual skews of the cards and the bodies grow
  with the perspective strength). The near and far planes are 0.6 and 2.5 of
  the distance to the eye.
- The cards stand by the donor's `card()`: from the floor under the bottom
  row of the sprite to the top one, leaning over its footprint. A bed lies,
  a person stands, a wardrobe hangs over its hex; from the game's point of
  view every texel is on its pixel.
- The card depth is the plane of its hex row (`rowPlane`, `rowTilt`,
  `stack`), so the sprites cover each other and the walls as in the game.

- The walls are as in the game view of the donor: the front face and the top
  (`wallFront`) and thin sheets. The backs, the add-ons and the slab ends
  (`wallRest`, `trims`) exist at the donor only for the first-person view;
  at ours they stuck out at the corners of the houses.

- A wall's top takes the depth of the front plane it was drawn on (`home` in
  `wallFront`, `walls.bin` version 2): what the game painted over with the
  wall (a post right behind it) stays covered. The wall faces are
  one-sided.
- What the donor's game view does not have and we do not need: drawn shadows
  on the floor, plants cross-wise, items on tables, box-bodies of furniture
  — that is the first-person view. Roofs are a separate task of the plan.

- The furniture — the donor's volumetric bodies (`props.py`,
  `render.js solid()`): a box or a cylinder, every point showing the pixel of
  the sprite it lands on from the game's camera; the faces in `walls.bin`
  (`solids.py`). As in the donor's game view, the faces take the depth of
  their card — the plane of its hex row, 2e-5 closer: the volume gives the
  shape, and what covers what is decided by the game's order. The object of
  a body is not drawn by a live card (`baked.bin` v2, SKIP); what stands on
  the body (an item on a table) is lifted onto it (LIFT). Animated ones
  (lockers, chests, a fridge opening) stay live cards.
- Translucent objects (steam, glass, energy, red, wall) — as in the engine
  (`_dark_translucent_trans_buf_to_buf`): the gray level of the texel 0..7
  mixes the effect's color with the background in sevenths. The smoke over
  the barrels is such steam; there were no barrels in the check frame, not
  verified by eye.

The earlier accepted 25° camera (section 10) is replaced by this.
Snapshots: `runs/p1-objects-20261018/devhero-20261018-212051-000.png`, after
the walls — `devhero-20261018-212530-000.png`.

## 13. The mouse and the interface in 3D — 2026-10-18

The mouse in 3D is driven by the game itself: hover, action icons, cursor
modes (the right button), walking, talking, "use", "get", examining. The
scene tells the game only two things:

- which hex is under the cursor — `tileUnderMouse` (the hex cursor, walking
  `_check_move`, the sfall `tile_under_cursor`): the scene floor under that
  pixel;
- which object is under the cursor — `gameMouseGetObjectUnderCursor`: the
  scene's pick buffer. The cards are drawn in the color of their number one
  pixel under the mouse, the nearest one is read and the frame is drawn
  again — a list of "what is under the cursor" from top down, like the
  engine's list of intersections, and the engine's rule is applied to it
  (the first opaque, not dead one). Walls and furniture bodies are picked by
  their own cards: from the game's camera the body covers the pixels of its
  sprite.

Over the scene the game's own picture is output: the bottom bar, the
messages, the cursor, the pop-up text, the action icons, the game's hex
cursor. The map window in 3D draws only that (`tileSetSceneOverlay`); the
rest is transparent where the map window is not covered by other windows.
The pop-up text and the hex cursor shift to where the scene shows their hex.
The dialog, combat and inventory are still shown in 2D.

The homemade walking of the 3D mode (a click → the hero's path) was removed:
the game replaced it.

Verified by the background probe (`tools/dev-snapshot.ps1 -Probe`,
`runs/p1-mouse-20261018/`): of the 12 residents in the frame 9 are picked,
3 stand behind a wall or a window and give the wall/window; a click on the
floor — the hero walked to the chosen hex 25875; a click on Aldo — the hero
ran up and a dialog opened. The snapshots show the game's hex cursor on the
floor and the "talk" icon over a resident.

Not done: the outline highlight of items (drawn only in 2D), picking and
ordering on maps other than Klamat, combat in 3D.
