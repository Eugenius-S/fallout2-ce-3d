---
id: 131
title: The wide loading screens show their own colours, not blue
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

Every loading screen (7 English, 7 Russian) shows the owner's artwork in its
own colours at 16:9 and 21:9.

## Context

- The owner, 2026-11-01, live on main `e434c327`: the loading screen was
  blue. A colour or palette conversion problem, like the earlier white
  speckle palette-index error that the corrected images fixed.
- The live `game/data/art/` files still match the old
  `rix3-install-package-backup-20261029-prepalette-fix` byte for byte. They
  do not match the corrected package in
  `game/user-assets/fullscreen-20261028/3440x1440-originals/rix3-install-package/`.
- Find whether the package files are wrong (channel order, 6-bit vs 8-bit
  palette values, palette slots the game reserves) or the game's splash path
  draws them wrong (the cover sizing of task 118, a palette that a later
  fade or the 3D build changes). Compare against a source PNG pixel by pixel.
- Fix the converter or the game, whichever is wrong. Write the corrected
  install package next to the old one; never commit the art.

## Acceptance

- A loading screen capture beside its source PNG for one English and one
  Russian screen at 16:9 and 21:9, in `runs/131-loading-screens-blue/`;
  colours match within the 254-colour quantization.
- Installation steps in the Log so Claude can install the corrected files
  into the owner's game.
- ctest passes; the original view regression stays pixel for pixel.

## Out of scope

The framing and crop of the loading screens (118), the menu (132).

## Log

### 2026-11-01 gpt

Done: The blue tint came from the installed RIX files, not `showSplash`.
SHA-256 checks confirmed all 14 files in `game/data/art/splash/` and
`game/data/art/russian/splash/` exactly match the pre-palette-fix backup.
That package had red and blue palette channels reversed and used reserved
pixel index 255. The already corrected RIX3 package has active indexes 1 to
254, 6-bit RGB triples, and mean PNG round-trip channel error 1.06 across all
14 screens. A Task 131 copy for Claude is at
`game/user-assets/fullscreen-20261028/3440x1440-originals/rix3-install-package-task131-corrected/`.

The checked-in side-by-side captures in `runs/131-loading-screens-blue/`
show English and Russian splash2 at 16:9 and 21:9 against their source PNGs.
They use isolated test-copy framebuffer captures from Task 118; its `showSplash`
implementation is unchanged between that capture build and `origin/main`
`e434c327`. Task 118 also captured all 14 corrected screens in both aspects.

Check: Release build passed; `ctest --test-dir build --output-on-failure`
passed 18/18; scene-export unit tests passed 88/88; regression unit tests
passed 12/12; `tools/regression.ps1` passed all 16 spots with zero changed
pixels, exporting to this worktree's `build/scene-cache`.

Claude install steps: first back up the 14 current RIX files. Then copy the
contents of the package's `art/splash/` to the game's `data/art/splash/` and
the contents of `art/russian/splash/` to `data/art/russian/splash/`, replacing
`splash0.rix` through `splash6.rix` in each folder. Verify the copied SHA-256
hashes against the package. No files in the owner's `game/data` were changed
for this task.

Next: Claude installs the verified package into the owner's game.
