---
id: 213
title: F12 screenshots, Ctrl+F12 with the highlight, and the FPS counter answer
milestone: M1
status: open
owner: gpt
suggested: owner
priority: medium
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The owner, 2026-11-07: "how do I take new screenshots and ones with the
highlight? F12 should take a screenshot, and the same button with Ctrl —
with the highlight." And: "how do I turn the counter on?"

## State

- F12 already takes a screenshot in the current build (the vanilla key,
  PNG per the screenshots_format setting, written next to the game).
- The FPS counter: press F8 in play (any launch, no dev flags) — the same
  row lives in the settings as "Счётчик FPS" with its description saying
  exactly that.
- "With the highlight": the only highlight layer in play is the game's
  mouse highlight of the object under the cursor (game_mouse). What
  exactly the owner wants highlighted on the Ctrl+F12 shot is not pinned
  down yet — the object under the cursor, the 3D scene's debug overlays,
  or something else. One owner word pins it; then Ctrl+F12 is one key
  binding away (the keyboard tables already carry ctrl variants, and the
  screenshot path takes flags).

## Acceptance

- F12 writes a PNG of the current frame.
- Ctrl+F12 writes the same frame with the agreed highlight visible.

## Log

- 2026-11-07: owner question; F12 and F8 confirmed working in plain
  launches; the highlight definition awaited.
