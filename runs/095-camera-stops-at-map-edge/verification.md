# Verification: task 095

Date: 2026-10-27. Branch: `task/095-camera-stops-at-map-edge`, base `cdb7b34e`.

## Change

Full 3D view no longer enables free scroll, so normal map-edge and object-blocker checks run. The separate expanded hero-distance limit was removed too: it let a wide 3D view scroll 240 px farther than Classic and produced different center tiles. Normal 3D and Classic scrolling now use the same original 480×400 limit. The fly camera remains on its independent `StaticFlyCamera` path.

## Verified on final executable

- Final Release executable SHA-256: `3f5e0256285bd5023290f809c9a26325a9587dd79d64db735cb79c6baa339cf0`.
- CTest: 18/18 passed.
- KLADWTWN screen-edge probes: 3D and Classic matched on all sides after 300 stable frames: Left 17526, Right 22872, Up 12876, Down 26722. All eight markers report `edg_enabled=1` and `edg_zone=1`; see `captures.json` and paired panels.
- KLADWTWN arrow-key probes at those endpoints: all eight (four directions × two views) passed with `moved=0`.
- KLAMALL left-edge probe on a second map: both views stopped at 18715. EDG was enabled and its zone selected. Center was (464,212), hero was (944,212), delta (−480,0); this stops at the shared hero-distance guard, not at the map boundary.
- The normal mouse-driven `-Probe` clicked a walk target: logs show the hero moving 25075→25875→25889. Its two snapshots both report center 24678. A separate normal walk animation traversed a 27-step path, hero 20100→16100, with center unchanged at 20100. Camera follow during walking was therefore not demonstrated.
- A `-FlyDistance 120` scene snapshot was rendered, but interactive WASD movement was not tested. The isolated test window is placed offscreen; CUA did not expose it for input.
- All 16 regression PNGs were RGBA-identical to `E:/Projects/fallout2-ce-3d-gpt-094/build/094-direct-base` (0 differing pixels; stable metadata differences 0). Baseline capture executable SHA-256: `aad846b198bf96d588d0157c7bd2fc535dd41558e41a3a2cbed35330d9352354`. Final capture executable SHA-256 is listed above. See `regression-rgba.tsv`; raw captures are in `build/095-final-regression-3f5e0256/`.

## Incomplete acceptance

- Object scroll-blocker path was not verified. The KLAMALL finder saw 479 ID 0x0C objects but found no adjacent scroll target outside the active EDG zone; log: `build/095-klamall-blocker-c8fccb7/process.stdout.log`. The EDG edge pair above confirms an active EDG scenario, not the object-blocker branch.
- Camera following during an actual click-driven walk remains unverified; in the tested route the center did not change.
- Interactive fly WASD remains unverified; only the camera's static free-distance path was exercised.

## Follow, fly input, and object-blocker investigation

- The completed mouse-click walk moved the hero from 25075 to 25889 while the center stayed at 24678. Source agrees with that result: `_dude_move` in `ce/src/animation.cc:3106-3124` schedules hero movement but never changes the camera center; `refreshForPresent` in `ce/src/scene/scene_view.cc:632-634` copies the existing `gCenterTile` into the 3D camera. The normal center-changing paths found are `mapScroll` (`ce/src/map.cc:625-653`), called by arrow keys and mouse-edge scrolling (`ce/src/game.cc:981-990`, `ce/src/game_mouse.cc:2617-2696`), and explicit centering such as Home (`ce/src/game.cc:793`). Thus this click-walk test does not establish autonomous follow; the examined normal movement path contains no such follow call.
- A safe synthetic WASD hold cannot be made with the existing `sfall_kb_press_key`: it pushes KEYDOWN and KEYUP consecutively (`ce/src/sfall_kb_helpers.cc:321-342`), while fly reads `SDL_GetKeyboardState` (`ce/src/devtools/devtools.cc`, `fly`). In the pinned SDL 2.32.8 source, keyboard state is updated by `SDL_SendKeyboardKeyInternal` before that function pushes an event (`build/_deps/sdl2-src/src/events/SDL_keyboard.c:816-920`); `SDL_PushEvent` only queues it (`build/_deps/sdl2-src/src/events/SDL_events.c:1156+`). Therefore replaying synthetic event structs alone cannot hold W for successive fly frames. Actual WASD remains unverified; no input was sent to the user's game.
- The object-blocker branch in `ce/src/tile.cc:607-620` is entered for scrolling only when EDG is inactive or no EDG zone is selected. I extracted the installed `game/mods/f2_res.dat` EDG entries into `build/095-edg` and parsed all 155: every file has one zone at each of the three elevations, including KLAMALL (`build/095-parse-edg.py`, result `(1, [1, 1, 1], 68, 68)`). `findZoneByPixel` returns the only zone whenever a floor's zone list is nonempty (`ce/src/map_edge.cc:154-177`); for these data, a loaded/enabled EDG map therefore keeps a zone selected, including outside that zone's rect. I did not verify the native object-blocker branch on a map whose EDG is active but zone is absent; the installed EDG data offers no such map/elevation combination. A map with no EDG file can naturally enter the object branch, but it does not test the requested EDG-active case.
- The one-use KLAMALL blocker search found 479 0x0C objects but no usable adjacent target outside the selected EDG zone (`build/095-klamall-blocker-c8fccb7/process.stdout.log`). Its temporary search harness was removed from the final source/tool changes; this result and the archive analysis above retain the reason the branch remains unverified.

Task 095 remains `doing`, `reviewed: no`; do not close until the remaining acceptance checks are resolved or explicitly accepted.
