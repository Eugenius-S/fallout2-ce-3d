---
id: 118
title: The owner's wide loading screens, main menu and death screen in the game
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The owner's 21:9 loading screens (English and Russian), main menu
background and death screen show in the game at 16:9 and 21:9, centred,
with the artwork's colours kept and the menu's buttons where they belong
and clickable.

## Context

- The owner extended the game's full-screen art by hand (the 2026-10-27
  inbox line on extending 4:3 art). gpt exported the originals and began
  installing the owner's images on 2026-10-28; its handoff is
  `game/user-assets/fullscreen-20261028/CLAUDE-HANDOFF.md` on the owner's
  machine (local, Git-ignored: game assets are never committed). Read it
  first. In short:
  - Loading screens: 14 RIX files (1120x480, en and ru) are installed as
    loose overrides under `game/data/art/splash/` and
    `game/data/art/russian/splash/`, byte-checked but never seen in a
    running game. They use 254 colours at 6 bits. With
    `splash_screen_size=1` (`ce/src/game.cc`, `settings.ui`) a 21:9 picture
    fits by aspect and leaves bars top and bottom on 16:9: the framing at
    16:9 is decided: the owner, 2026-10-28, live, saw the wide pictures fit
    to the screen's width; they must fit to its height instead: the picture
    fills the screen top to bottom at its own aspect, centred, and the sides
    beyond a narrower screen are cut off. The same for the menu and the
    death screen.
  - Main menu: not installed. `ce/src/mainmenu.cc` loads `HR_MAINMENU.FRM`
    through the global indexed palette, and a plain conversion of the
    owner's 1820x768 picture loses colour visibly
    (`game-staging/menu-preview.png`). The control panel moves with the
    background's aspect. Keep the artwork's colours (a true-colour path for
    this background, not the game palette) and keep the buttons on their
    painted places.
  - Death screen: the owner's 21:9 version (1916x821) is only in the
    collection, not installed.
  - The first export's RIX PNGs have a palette-index error (white specks);
    the corrected images are in `wallpapers-selected/`.
- Never commit game files; the code that shows the art is ours and goes in
  the PR. Test in an isolated instance, never the owner's running game.

## Acceptance

- Screenshots of each loading screen (en and ru), the main menu and the
  death screen at 16:9 and 21:9, from a running game, in
  `runs/118-wide-fullscreen-art-in-game/` (our captures only, cropped to
  show the framing, no full-size copies of the art).
- The menu's buttons click at both aspects; the 4:3 original art still
  shows as before when the wide files are absent.
- ctest passes; the original view regression stays pixel for pixel.

## Out of scope

Making new wide art, the ending slides, movies.

## Log

### 2026-10-29 gpt
Cause: the wide splash was scaled to 1260x540 logical pixels at 16:9, then
passed to `_GNW95_ShowRect` with its full width for a 960-pixel screen. That
blit writes wider than the destination pitch and extends past the final row.
The 1120x480 source was confirmed at runtime as `art\\splash\\splash2.rix`.

Fix: crop the scaled splash to the screen before blitting, using the scaled
buffer width as the source pitch. The source rectangle and destination are
both clamped to their buffers and centered. The menu and death paths were
checked: their scaled blits clip to the target surface and their loops bound
accesses by the allocated surface dimensions.

Check: the old blit was tried in the isolated game at 16:9 with scale 2 and
scale 1; it reached the saved map but did not reproduce exception 0xc0000374.
AddressSanitizer was not enabled because `tools/configure-build.ps1` has no
ASan option; the changed path was checked against the surface and source
bounds. The final uninstrumented EXE loaded slot 1 to `KLADWTWN.SAV` at
1920x1080 and 2560x1080. Release build passed, CTest passed 18/18, and the
pixel regression against current `origin/main` passed 16/16 with zero changed
pixels. Main sidecars identify `0ebfd1f5`; branch sidecars identify `12ffd925`
and final EXE SHA256
`04291bfa0212efa578e6eda199c9893e80662d65ca9e0b77c194944fea7cbdba`.
Runtime loading and menu captures for both aspects are in this folder.

No game assets were committed. The owner game and its `mod-cache` were not
written.

### 2026-10-29 gpt follow-up
The owner's 1120x480 splash is 2.333:1; 2560x1080 is 2.370:1. Height-fit
left about 20 physical pixels at each side. `splash_screen_size=1` now uses
cover sizing, rounds up to avoid seams, and center-crops the excess. At 21:9
the 1280x549 scaled source is cropped to 1280x540; at 16:9 the 1260x540
source is cropped to 960x540.

The menu loader now tries the existing owner wallpaper
`user-assets/fullscreen-20261028/wallpapers-selected/hires/menu/art__intrface__hr_mainmenu__d0_f0_21-9.png`
before the legacy image. It is 1820x768 and matches the game-folder asset by
SHA256. The fallback remains the legacy wide image, then the stock menu art.
No copy into `data/art/intrface` or any owner game file was needed. The test
copy showed the exact wallpaper at 16:9 and 21:9; its capture identity and
the final no-hook slot-1 EXE identity are in the matching JSON sidecars.

Final Release build and CTest passed 18/18. Pixel comparison against fresh
`origin/main` (`0ebfd1f5`) passed 16/16 with zero changed pixels; all test
captures used `build/scene-cache`. The uninstrumented final EXE loaded slot 1
to `KLADWTWN.SAV` at both aspects. The existing `game/user-assets` is where
the owner asset lives; the updated executable reads it from the game working
directory, so the owner will see it after updating the build and restarting.

### 2026-10-29 gpt ultra-wide follow-up
The owner asked for a supported art viewport up to 8:3. Added a shared
viewport calculation used by loading, main-menu art and death art. Displays
wider than 8:3 keep a centered 8:3 area and black side bars on the full-size
surface; the compositor still receives the full display-sized surface.

The 8:3 menu crop also exposed an unsafe text origin: the cover-sized art can
have a negative top edge while the menu labels remain unscaled. Menu offsets
now account for that crop; all six button rectangles are checked against the
surface, and native-size labels are drawn only with a valid vertical span and
an X origin clamped to the surface. Panel blits and mask writes use their
existing clipped bounds. Geometry was checked at 640x480, 960x540, 1920x720
and 2560x720 logical resolutions; all button and label rectangles remain
inside the surface.

Isolated runtime captures show loading, menu and owner death art at
3840x1440 (8:3) and 5120x1440 (32:9). The 8:3 images have no side bars; the
32:9 images have symmetric black bars. The capture used temporary test-only
hooks (EXE SHA256 `622c3709cd389dff25f8cea18d553b3290c420b833a1925d6cb48fad60ed9e48`);
the hooks were removed before the final build. Contact sheets for 16:9,
21:9, 8:3 and 32:9 are the four JPEGs in this folder, each under 1 MB.

The final uninstrumented Release EXE (`6cfd6cb773118730031828f282563821d3fd29a780780c0275ad9a5951e5c82f`)
loaded slot 1 to `KLADWTWN.SAV` at 1920x1080, 2560x1080, 3840x1440 and
5120x1440. CTest passed 18/18. Direct map captures using
`build/scene-cache` matched fresh `origin/main` (`0ebfd1f5`) pixel for pixel:
16/16, zero changed pixels. No game files were committed; the owner game and
its `mod-cache` were not written.

### 2026-10-29 gpt follow-up: stretch art and replacement RIX
Menu and death owner art now stretch over the full output surface at every aspect. Loading art keeps its proportional cover and 8:3 viewport. The menu layout uses the full surface when owner RGBA art is active so its panel and buttons stay inside the image. The death image scaler writes exactly `screenWidth * screenHeight` pixels and clips the subtitle mask to its allocated surface.

The owner added 14 loading PNGs under `game/user-assets/fullscreen-20261028/3440x1440-originals/loading/`: 7 English and 7 Russian, each 1147x480. The old 1120x480 files remain untouched. Every new image's first 1120 columns match the older image pixel-for-pixel, with 27 pixels added on the right. Prepared RIX3 files and installation notes are in the ignored owner asset directory `game/user-assets/fullscreen-20261028/3440x1440-originals/rix3-install-package/`. Each RIX is 1147x480 and 551338 bytes; the PNGs were quantized per image to 254 colors with FFmpeg, with no dithering, then written in RIX3's 6-bit palette format. They were copied only into `build/test-game-118` for runtime testing. English splash0 rendered at the 3440x1440 output target and Russian splash0 at 1920x1080; the actual image decoder loaded both prepared RIX files.

Menu runtime captures in the same isolated test copy show full coverage and visible controls at output targets 1920x1080, 3440x1440 and 5120x1440. With `main_menu_scale_mode=0`, the owner wallpaper also filled a 3840x2160 target (surface 1920x1080) with controls visible. CPU captures are logical size at `--dev-scale=2` (960x540, 1720x720 and 2560x720); these preserve the tested aspect and screen geometry. Evidence is `menu-16x9.jpg`, `menu-43x18.jpg`, `menu-32x9.jpg` and `loading-43x18.jpg`, with EXE identities in `runtime-assets.json`.

Death was not visually verified: the slot-1 isolated runs loaded the map, but the temporary force-death harness did not produce a capturable `showDeath` frame. The code change is therefore not accepted as runtime-tested and PR #157 remains a draft. All temporary capture/force hooks were removed before the final build. The final Release EXE SHA-256 is `07E49F2517CCE6C74B86F79FB57929676965F9C4941D141B97724B6E2169C450`. CTest passed 18/18. The full regression export wrote only to `build/scene-cache`; its tracked-reference summary flags `klagraz-e0` (183 pixels) and `klaratcv-e2` (6299 pixels). Direct comparison of all 16 captures to the fresh `origin/main` baseline images in `runs/118-regression-main-reference/` was exact: 16/16, zero changed pixels (baseline commit `0ebfd1f5`).

No files were copied into owner `game/data`; owner settings and `game/mod-cache` were not written. No game assets are staged for Git.



Final direct comparison used branch run `runs/regression/20261029-160517-034-69080` and fresh main reference set `runs/118-regression-main-reference/`; evidence is in `runs/118-wide-fullscreen-art-in-game/regression-vs-origin-main.json`.

### 2026-10-29 gpt runtime follow-up
The current branch commit is `7aa13c07`. I tested the death display in the
isolated copy using a temporary test-only switch that set `DAM_DEAD` immediately
before the normal main-loop death check. The normal `showDeath` path displayed
the owner image edge-to-edge at output targets 3440x1440 (surface 1720x720) and
5120x1440 (surface 2560x720). The captures are `death-3440-diagnostic.jpg` and
`death-5120-diagnostic.jpg`; their JSON sidecars record both temporary capture
EXE identities and the final clean Release EXE identity.

The isolated config had `subtitles=1`, but no caption appeared in either death
frame. I did not isolate whether the selected narrator text file was missing,
and did not trigger death through ordinary combat. The menu artwork is still
visible in the earlier 16:9, 43:18 and 32:9 captures; the new click attempt did
not reach the test window, so menu interaction is not claimed as checked in
this follow-up. The test copy contains all 14 prepared splash RIX files (7
English and 7 Russian, 551338 bytes each); I did not repeat the loading capture
in this run.

All temporary source hooks were removed. The final clean Release EXE is
`EECAD8D7654A87C4F70C66681CE5C896B096DF79FDA03116C6C893B04A8ABEEA`; its copy
in `build/test-game-118` has the same SHA-256. CTest passed 18/18. The prior
16/16 pixel comparison used this same tracked commit and remains the regression
result. Owner `fallout2.cfg` timestamp remained 2026-10-29 13:33 and owner
`game/mod-cache` files remained at or before 13:14, before the 16:23 isolated
runs. No owner game file was written. Keep PR #157 draft because subtitle text
and a real menu click remain unverified.

### 2026-10-29 gpt palette and aspect correction
The owner's updated loading PNGs were exported with red/blue palette channels swapped and pixel index 255 in active data. `showSplash` treats indices 0 and 255 as reserved and swaps them, so 255-colored pixels became black; channel reversal caused the strong blue tint on splash1/2/4/5/6. Legacy RIX confirms RGB palette triples, 6-bit DAC values expanded by `<<2`, and active indexes 1..254. The corrected package was regenerated in RGB order with 0/255 reserved, nearest-pixel assignment against the final emitted 6-bit palette, and all 14 PNG-to-RIX round trips audited. Mean RGB MAE is 1.06, individual images 0.71 to 1.34; this is comparable to the older original game RIX palette error. The original incorrect package is backed up at `game/user-assets/fullscreen-20261028/3440x1440-originals/rix3-install-package-backup-20261029-prepalette-fix/`. Updated package and metrics remain in the Git-ignored owner asset folder; the corrected RIX files were copied only to `build/test-game-118/data/art/` for runtime checking.

Menu and death art now use centered proportional cover instead of stretching. Menu source pixels are mapped through the full virtual cover dimensions, with 64-bit source coordinate arithmetic; UI remains at native size. Death art now uses the same proportional centered cover and checks output allocation and scaled dimensions before writing. Loading still uses the existing 8:3-capped cover viewport.

Runtime evidence in `runs/118-wide-fullscreen-art-in-game/runtime-palette-fix/` contains English and Russian loading screenshots captured from the isolated test copy at logical 1720x720 for output 3440x1440, plus a 1720x720 main-menu frame captured after palette fade. The loading screenshots match the updated `splash2` source art at RGB MAE 1.79 after nearest resize; they have no blue channel swap. Menu art covers all edges and buttons/text are visible at native size. Temporary capture hooks were removed before the final clean build.

Not verified: a current-code death-screen runtime capture and death subtitle. Keep PR #157 in draft. Final clean Release EXE SHA-256 `CBFB53A9D400E6758FADF479DDF157510ECA2922E89EA9E5FF097B4FEFF2B1E1`; CTest 18/18. Full regression exported only to `build/scene-cache`; direct comparison against fresh `origin/main` `0ebfd1f5` was 16/16 exact with zero changed pixels. `tools/regression.ps1` still reports the known old-reference differences for `klagraz-e0` (183) and `klaratcv-e2` (6299). No owner game/config/cache files were written.

### 2026-10-30 gpt runtime verification follow-up
The current `origin/main` build passed CTest 18/18. A fresh full regression run
used only `build/scene-cache`: 14/16 references passed. `klagraz-e0` differed
by 183 pixels and `klaratcv-e2` by 6299; the other two nonzero results were
within the checker threshold. This is a main self-regression, not an
independent branch-versus-main capture: this verification branch has no code
diff from the tested main.

In an isolated test copy, the normal `showDeath` path rendered the owner's
death art and subtitle after `renderPresent` at 16:9 and 21:9. The two compact
captures and full provenance are `runs/118-wide-fullscreen-art-in-game/death-runtime-current-16x9.jpg`,
`death-runtime-current-21x9.jpg` and `runtime-verification-20261030.json`.
The copied slot-1 save still fails at load function #3, so this death check used
a temporary test-only `DAM_DEAD` trigger on a fresh dev character, not slot 1.
All source hooks were removed before the clean Release build. The test copy
alone was used; owner game files, configuration and `game/mod-cache` were not
written.

The isolated menu helper reached engine hit-testing and queued `KEY_O` (111)
on Options mouse-up, but the Preferences transition was not visually confirmed
and no physical click was tested. Keep this task `doing`; do not mark the
remaining interaction acceptance complete. PR #157 is already merged; this
follow-up records verification and its remaining limit.

### 2026-10-30 gpt Options transition verification
The previous entry's input test was incomplete. A revised in-process test used the real Options button hit-test and queued its resulting event 111 through `inputGetInput`; the normal main-menu dispatch entered `doPreferences`, whose rendered framebuffer was captured. This was confirmed in the isolated copy at 1920x1080 (16:9) and 2560x1080 (21:9). The screenshots and provenance are in `runs/118-wide-fullscreen-art-in-game/options-transition-20261030/`.

This verifies the engine input and Preferences transition path, not a physical mouse click. The game owner's process, configuration and cache were not touched. The run log included a `ce.dat` open warning, although the Options screen rendered successfully. Temporary diagnostic hooks have now been removed from the source.

The existing loading captures do not establish that every one of the 14 English/Russian RIX files was displayed at both 16:9 and 21:9. Keep the task `doing` and its follow-up PR draft; do not claim complete acceptance. Re-run the clean build and CTest after hook removal. The latest full pixel comparison is the main self-regression recorded above; this branch has no game-code diff.

### 2026-10-30 gpt full loading and menu input verification
All 14 installed English/Russian loading RIX files were selected by the normal startup setting and rendered by `showSplash` in an isolated game copy. Captured the actual framebuffer after palette fade for each splash0–6 at 1920x1080 and 2560x1080 (28/28). The four contact sheets and per-frame source RIX hashes, dimensions, screenshot hashes, EXE SHA and sidecars are in `runs/118-wide-fullscreen-art-in-game/loading-audit-20261030/`. Visual review confirms the intended language text and image colors in both aspects.

The six main-menu buttons (Intro, New Game, Load Game, Options, Credits, Exit) were each exercised at both aspects (12/12). A temporary in-process harness positioned over the actual button rectangle, simulated the mouse-down, observed engine hit-test result 1111, and verified that the ordinary menu loop consumed the expected key and entered its matching action case. This did not use the OS cursor or focus another window. A physical mouse click was not tested. The per-run stdout traces and EXE identity are in `runs/118-wide-fullscreen-art-in-game/options-transition-20261030/button-dispatch-results.json`; the two actual Preferences screen frames are beside it.

All temporary source hooks were removed after capture. The clean Release build and CTest passed 18/18. The final clean EXE SHA-256 is recorded in the current follow-up evidence. A fresh full regression run exports only to `build/scene-cache`; main self-regression is 14/16: 183 pixels at `klagraz-e0`, 6299 at `klaratcv-e2`, all other captures pass (including 3 and 1 pixel changes below threshold). This is the known stale-reference delta on current main; this verification branch has no production source diff from `origin/main`. Earlier verified 4:3 fallback, menu/death artwork at 16:9 and 21:9 remain in their prior evidence folders. No owner game process, settings, or cache were touched.

Final post-clean run: `tools/regression.ps1` used the clean Release executable SHA-256 `B9ED2793A57640F7B2DD1E3FB8815C76C71266B5C68DDF7BE850C0A5EA55B56B`, exported only to `build/scene-cache`, and captured under `runs/regression/20261030-232223-381-61624/`. The current-main reference checker again reports 14/16: `klagraz-e0` +183 and `klaratcv-e2` +6299; all other spots pass (including 3- and 1-pixel differences under threshold). `origin/main` is `3a59aefb`; `git diff origin/main...HEAD -- ce/src` is empty. Treat the two counts as current-main self-regression deltas against existing references, not as task-caused changes.
