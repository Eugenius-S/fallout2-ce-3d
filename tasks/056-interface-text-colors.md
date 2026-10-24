---
id: 056
title: Sharp interface text colors
milestone: M1
status: doing
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: [055]
reviewed: no
---

## Goal

Recolor only semantic green font-101 sharp text to #3FB419, with the lower HUD
message log at #4BD51F. Preserve all non-green palette colors, fades, and
relative dimming.

## Context

- Font 101 already uses embedded Bold 700 from task 055.
- Sharp rendering records the source palette index and live window ownership.
- The message log is drawn in `displayMonitorRefresh` into the interface bar
  buffer at its lower HUD log region. Mark that draw site with a scoped role so
  copied records keep the exception without relying on RGB hue or character
  screen detection.
- Green reference: `runs/inbox/2026-10-24-sharp-green-reference.png`.
- Non-green black text reference: `runs/inbox/2026-10-24-nongreen-black-reference.png`.
- Separate black dialog background request and reference are recorded in
  `tasks/inbox.md` and `runs/inbox/2026-10-24-dialog-black-background.png`.

## Acceptance

- Semantic source green palette states are identified by their canonical
  palette indices at capture time, never by rendered RGB hue. Include GREEN,
  LIGHT_GREEN variants, DARK_GREEN variants, and SAGE_GREEN. Keep GRASS_GREEN,
  YELLOW_GREEN, and OLIVE unchanged because they carry inactive/warning meaning.
- Font-101 green records render at normal full intensity as #3FB419; only the
  lower HUD message log renders at #4BD51F.
- Source-palette dimming and palette fades reduce the target green proportionally.
- White, yellow, red, grey, black, and other non-green font-101 records retain
  their original palette colors and fade behavior.
- Tests cover both exact full-intensity targets, dimmed targets, black/white/
  grey/yellow/red preservation, black and half palette fades, and copy/owner
  propagation for the HUD log role.
- Russian inventory, dialogue, character, and lower HUD log snapshots show the
  recolored green at scale 3. English barter confirms white `$0` totals remain
  unchanged. Verify exact RGBs from prepared overlay opaque pixels and decoded
  output pixels, accounting for antialiasing at glyph edges.
- Release build, CTest, scene exporter tests, regression unit tests, and the
  official 16-point sharp-off regression pass.

## Out of scope

- Font face, size, metrics, placement, brightness settings, game palettes,
  original pixels, or non-green sharp text colors.
- The black background around the dialogue screen; it remains an inbox item.

## Log
