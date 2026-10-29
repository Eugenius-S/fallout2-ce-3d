---
id: 118
title: The owner's wide loading screens, main menu and death screen in the game
milestone: M1
status: done
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
