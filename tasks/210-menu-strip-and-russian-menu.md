---
id: 210
title: No stray strip in the menu at any resolution, and the settings menu speaks Russian
milestone: M1
status: open
owner: gpt
suggested: owner
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The owner, 2026-11-07, after the 208 fixes: "the white strip in the menu is
still there" (now at 3440x1440 borderless), and "the interface in my current
build must be Russian" — the settings rows showed "< Window >" in English.

## Cause

The strip was never the Windows border alone: the menu art itself carries a
light left column (about 134,138,143 against the dark 15,23,32 next to it).
At 1920x1080 the crop-fit hides that column; at 3440x1440 the screen is a
hair wider than the art (2.389 vs 2.370), the crop becomes vertical, and the
art's left column lands on the screen edge. The repro at the owner's old
resolution could not see it.

## Fix

- The menu background mapping samples the art's interior, skipping the
  outermost pixel ring, so baked edge junk never reaches the screen.
- The settings menu catalog gains the display rows and the mode choice
  names in Russian (Resolution, Window mode, Fullscreen, Window,
  Borderless, the scale row), and the descriptor localization runs for
  every section, so translated rows show Russian everywhere and rows
  without a pair keep English until their pairs land.

## Acceptance

- A menu capture at 3440x1440 shows no light column at the left edge.
- The display settings rows render in Russian with the game's language.

## Log

- 2026-11-07: owner report; the art column found by measuring the PNG's
  edge columns against the screen aspect; both fixes shipped the same day.
