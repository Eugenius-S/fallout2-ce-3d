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

- F12 writes a PNG of the current frame with the metadata JSON next to it.
- Ctrl+F12 enters the region mark: rectangles over the held frame, saved
  as the marked PNG plus the JSON with the regions under them.

## Log

- 2026-11-07: owner question; F12 and F8 confirmed working in plain
  launches; the highlight definition awaited.
- 2026-11-07, later: the owner pinned it — "выделение рамкой, просто чтоб
  агент понял куда смотреть" — and reminded the metadata requirement. The
  feedback-shot feature (183) already does both on F9/Ctrl+F9; the same
  flow now answers on F12 (shot with metadata) and Ctrl+F12 (the region
  mark). The bare vanilla screenshot key is unbound so the keys reach this
  path; the F1 list and the key comments updated.

- 2026-11-07: owner question; F12 and F8 confirmed working in plain
  launches; the highlight definition awaited.
