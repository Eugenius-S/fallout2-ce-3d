---
id: 118
title: The owner's wide loading screens, main menu and death screen in the game
milestone: M1
status: todo
owner: none
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
    16:9 is open (crop the sides to fill, or bars), decide with the owner's
    note in the Log.
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
