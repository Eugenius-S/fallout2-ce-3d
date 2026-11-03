---
id: 151
title: The message log lines all start at the same left edge
milestone: M1
status: done
owner: glm
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

Every line in the interface bar's message log (the green text window at the
bottom left) starts at one left edge: the bullet of each message and the
wrapped continuation lines line up.

## Context

- The owner, 2026-11-02, live on main `a4d47b26`, Russian, in combat:
  `runs/inbox/2026-11-02-message-log-indent.png`. The first message's
  bullet sits at the window's left edge; the next four start one character
  to the right ("•Цель вне досягаемости." vs " •Вы промахнулась.").
- Find whether the offset comes from the message text (a leading space in
  the game's strings), from the sharp interface text (054-056) placing the
  lines, or from the original layout. Fix it where it comes from, as a rule
  for every message, both languages.

## Acceptance

- before | after of the log with several messages, English and Russian, in
  `runs/151-message-log-left-aligned/`.
- ctest passes; the regression stays pixel for pixel outside the log.

## Log

- 2026-11-02 Cut from the owner's live find.

### 2026-11-03 glm
Done: the offset came from the sharp interface text pass, not from the game's
strings or the original layout. Vanilla draws font 101's bullet (cp1251
0x95) with no side bearing, its ink on the pen; the sharp pass re-renders the
runs with the mono interface font, whose U+2022 has a wide advance and a
bearing, so every bulleted message sat right of the wrapped continuation
lines. `renderSharpText` now draws a log run's leading bullet ink on the pen
and re-flows the line from the first letter's vanilla position
(`run.characterX[1]`); a rule for every message, both languages; lines
without the bullet and the vanilla one-pixel stair are untouched. The log
fixture seeds the config's language now (English filler added). Measured
leftmost ink per line moved from 209..219 to 206..216 physical px (scale 2),
the +2 per line left is the vanilla stair; sharp text off was already
aligned. Evidence: `runs/151-message-log-left-aligned/` (before | after,
English and Russian, plus the numbers). 18/18 ctest, 97 + 12 python tests,
export clean, 16/16 regression spots at 0 changed pixels.
Next: the owner's visual check (user-check), then the merge.
Check: the four crops in `runs/151-message-log-left-aligned/`.
