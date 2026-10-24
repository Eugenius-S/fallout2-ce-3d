---
id: 054
title: Sharp small interface text everywhere
milestone: M1
status: doing
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: [025]
reviewed: no
---

## Goal

Replace the original small interface font with the high-resolution text pass
wherever the game actually draws font 101. Keep the game's original colors,
palette effects, placement, and layout. Other font IDs and text baked into art
remain unchanged.

## Context

- The owner requests the same small green interface font everywhere, including
  inventory, Pip-Boy, character/status screens, conversation and barter text,
  amounts such as `$0`, and message logs.
- The stable identity is the active font ID 101, not green pixels or a
  conversation tag. The modern font manager covers IDs 100 through 110.
- Barter totals are drawn in white with font 101 in
  `ce/src/inventory.cc:barterDisplayTables`; a green-only filter misses them.
  Other font-101 text also uses selection, warning, and faded palette colors.
- Fonts 102 and 103, including larger headings and labels, must remain on the
  original path. Text embedded in FRM artwork is not a font draw and is out of
  scope.
- Preserve the current feature defaults. Do not add a brightness setting or
  change brightness, font size, or weight during the coverage stage. Those
  visual changes can be considered after coverage is reviewed.
- Task 025 remains the historical implementation record. This task broadens
  its eligibility and setting label; do not rewrite task 025's history.

## Acceptance

- The existing sharp-text setting applies to every genuine font-101 draw and
  keeps each run's original `ColorWithFlags`. Font IDs 102, 103, 104 and other
  non-101 fonts stay on their original rendering path.
- Tests cover font 101 using green, white, yellow, and red; they verify the
  original palette flags and font draw metrics remain intact. Tests also prove
  that IDs 102, 103, and 104 are not selected for restoration or sharp rendering.
- Real engine UI evidence covers inventory, Pip-Boy, character/status,
  conversation, barter totals, and message log. Use English and Russian at
  representative x2 and x3 scales; verify barter `$0` uses the same font 101
  path. Screenshots must come from the official hidden snapshot helper and
  loaded game resources, not synthetic UI buffers.
- The sharp-text-off 16-spot regression remains pixel-identical to its
  existing references.
- The setting label and description describe small interface text generally,
  without changing its default or adding a new preference.

## Out of scope

Other font IDs, FRM artwork text, changing brightness, increasing font size,
or changing font weight during this coverage stage.

## Log

### 2026-10-24 gpt
Done: accepted the font-101 scope and created task 054 on a branch from the
current `origin/main`.
Next: implement the minimal font-101 eligibility rule and general setting
label, add focused tests, then build and run the unit suite before adding any
new live UI fixtures.
Check: read-only audit found font-101 use across inventory, Pip-Boy,
character/status, dialogue, barter, message log, and other small interface
labels. Barter totals use white; exact font identity preserves their palette.

### 2026-10-24 gpt (eligibility stage)
Done: changed sharp eligibility to depend only on font ID 101, preserved each
run's original color and flags, and broadened the setting label and description.
The focused capture test covers green, white, yellow, and red font-101 text,
excludes font IDs 102 through 104 from sharp rendering, and checks unchanged
font metrics and copy propagation.
Next: parent review this bounded stage before adding real UI fixtures.
Check: Release build using `tools/configure-build.ps1`; CTest 14/14 passed.
No runtime screenshots or new screen fixtures have been added.

### 2026-10-24 gpt (real UI fixture implementation)
Done: added opt-in `--dev-ui-fixture` modes for inventory, Pip-Boy, character,
barter, and message log. They use normal engine key routes and visible window
records. Barter reuses the reachable Sulik conversation and the real B key
path; it requires two white `$0` totals. Message log uses the existing Classic
2D map path so the vanilla frame includes the HUD, then validates a new current
date record created through the stock slash key. Fixtures require the
copied screen-buffer records to match the expected owner window generation,
have visible coverage, and contain overlay pixels when sharp text is enabled.
The Pip-Boy fixture enables the existing start option in memory for the
isolated process only, then opens the screen through the stock P-key path and
selects the Status page through its normal button event.
The helper checks the screenshot's actual resolution, scale, and sharp-pass
metadata.
Next: parent review the fixture code and helper checks before any runtime
matrix launch.
Check: Release build using `tools/configure-build.ps1`; CTest 14/14 passed and
the PowerShell helper parsed successfully. No runtime fixtures were launched.
---
