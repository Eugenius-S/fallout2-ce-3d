---
id: 089
title: Black dots on the toxic slime and black main menu button text
milestone: M1
status: done
owner: glm
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The toxic slime shows no black dots, and the main menu button labels are
yellow, as in the original game. Find where the black comes from and fix
the cause, not each place.

## Context

- The owner, 2026-10-26, in the live game: the green slime is speckled with
  black pixels (`runs/inbox/2026-10-26-slime-black-dots.png`); the main
  menu button labels, yellow in the original, come out black. The owner
  thinks it is the same cause.
- Likely suspects, to check first: palette entries that turn black on their
  way into the 3D picture (the cycling colours the slime uses, unmapped or
  zeroed palette entries, as the 073 fixture showed: it restored unmapped
  `_cmap` entries as zeros); a texture cached before the palette was filled
  and never refreshed; an index the sprite alpha or the sharp text layer
  takes for transparent or black.
- Compare with Ctrl+F10 (the original view) on the same frame: if the
  original view is clean, the cause is in the 3D path.

## Acceptance

- The cause named in the Log, with the palette index (or indices) that
  went black.
- Before | after in `runs/089-black-dots-slime-menu/`: the slime close up
  (live cycling) and the main menu buttons.
- The original view regression stays pixel for pixel.

## Out of scope

The glow strength of the slime (073, 086).

## Log

- Two independent causes, both palette-index level, both fixed at the
  source. Done 2026-10-27, branch task/089-black-dots-slime-menu.
- MENU, index 228: `mainMenuDrawButtonLabels` used `COLOR_BLACK` both as
  the "no value" sentinel for sfall's `big_font_color` and as the config
  default. On the vanilla palette `COLOR_BLACK` is `_colorTable[0]` = 228
  (the mapped pure-black entry), never zero, so the override guard was
  always true and the labels drew with index 228 = black. The footer
  function already used `COLOR_FIRST` (0) as its sentinel, which is why
  the version and BUILD HASH lines stayed yellow on the same screen.
  Upstream fallout2-ce inits the sentinel to 0 and reads the key without
  a default. Fix: sentinel 0, `configGetEnum` without the default; a
  configured colour still wins.
- SLIME, indices 229-232: `color.pal` marks the whole cycling tail
  (indices 0 and 229..255) unmapped, so `colorPaletteLoad` zeroes those
  `_cmap` entries, and every `paletteSetEntries(_cmap)` (each map load)
  pushes those zeros into the system and display palettes until a cycle
  ticker tick refills them. The goop FRMs (goop01-04, the toxic slime on
  klatoxcv) paint 8-11% of their pixels with indices 229-232; the 3D
  object bake converts indices through `directDrawGetPalette()` at scene
  build - inside that zero window at every map load - and the texture
  cache never re-bakes, so the cycling pixels freeze black. Fix: new
  `colorCycleSeedPalette` (cycle.cc) writes the cycling ranges' rest-phase
  colours into any palette where they are unmapped, called from
  `colorPaletteLoad`, so `_cmap` and every restore carry real cycling
  colours. Entries the loaded palette maps to its own colours (death.pal
  uses the whole range) keep those. The ticker keeps animating as before.
- Evidence, `runs/089-black-dots-slime-menu/`: `089-sheet.jpg` (menu
  labels black | yellow; the slime speckled | clean); full frames
  `menu-fork.png`, `menu-fork-after.png`, `slime-before-still.png`,
  `slime-after-still.png` (same kladwtwn view, hero 25075, zoom 3, a
  spawned `toxic-goo` fixture object); `slime-unfixed-live-cycling-green.png`
  documents why a late bake hides the bug: with the ticker running the
  display range is greens again long before any late fixture spawns, so
  only a bake inside the zero window - the live game's map-enter scene
  build - shows the dots. The `toxic-goo` entry in the weapon-effect
  fixture catalog (goop01.frm, scenery 996) and the `--dev-cycle-zero`
  freeze switch are test fixtures, not game changes.
- Checks: ctest 16/16; regression corrals (kladwtwn, the fire-barrel
  spot) pass 0 changed pixels on both the pre-fix and post-fix builds.
- Infra finds on the way, both dev-only: (1) the `--dev-map` startup
  travel gave up on the first refusal (the new game opens on a movie) -
  `applyStartupTarget` now retries from `devtools::update()` and reaches
  the target map; F7 duplicates the panel's Alt+D because posted window
  messages cannot hold Alt; throttled state prints in the travel path.
  (2) `tools/game-input.ps1` also matches the `fallout2-ce-test` process
  name.
