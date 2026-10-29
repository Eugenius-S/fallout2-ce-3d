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
Done: Added fill-height framing for wide loading screens, menu art and the death screen. Added an RGBA background path for the owner's local menu and death PNGs while keeping indexed menu controls and subtitles visible. Original 4:3 framing still uses aspect fit.
Next: Resolve the two repeated regression capture failures and pixel mismatches, then capture every required loading screen, menu and death screen at both aspects and verify menu clicks.
Check: Release configure-build passed. `ctest --test-dir build --output-on-failure` reports 17/18 passed; `settings_3d_tests` fails on its missing `clean_edges` default assertion. Python scene-export tests passed 67/67; regression tool tests passed 12/12. Full `tools/regression.ps1`: 9 pass, 5 fail, 2 error. Failing changed-pixel counts: entrance 191092, map-edge 453853, klacanyn-e0 246157, klagraz-e0 42372, klaratcv-e2 6299. Capture errors: residents and klatoxcv-e2, each after two 90-second attempts. No required art screenshots or button-click checks have been completed.
