---
id: 140
title: Letters in the dialogue answers are not cut at the top or bottom
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

Every letter of the sharp interface text is drawn whole: no tops or tails of
letters cut off where a line meets the next one or the edge of its box.

## Context

- The owner, 2026-11-01: `runs/inbox/2026-11-01-dialogue-letters-cut.png`,
  the dialogue answer list in English: the bottom of the line above
  ("What are you doing here?") is sliced off, so its letters lose their
  lower half; the same happens to tall letters and descenders elsewhere.
- The sharp text pass (025, bold font 055/074, sizes 054/057) draws each
  line into a box sized for the game's original pixel font; the larger
  outline font overflows it and the next line or the box edge clips it.
  Find where the clip happens (line height, the box rectangle, the order
  lines are drawn) and fix it there, not by shrinking the font.
- Check the other places the sharp text draws: the message window, the
  character screen, inventory, barter, Pip-Boy, the options rows, in English
  and Russian (Cyrillic has tall capitals: Й, Ё, Щ, Д, Ц, У).

## Acceptance

- before | after crops of the dialogue answers, the message window and the
  character screen in English and Russian at x2 in
  `runs/140-dialogue-letters-cut/`, no letter cut.
- ctest passes; the original view regression stays pixel for pixel.

## Out of scope

The fractional scale (139), new fonts, text layout changes beyond the clip.

## Log
