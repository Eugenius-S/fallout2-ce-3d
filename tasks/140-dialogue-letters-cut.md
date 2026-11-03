---
id: 140
title: Letters in the dialogue answers are not cut at the top or bottom
milestone: M1
status: todo
owner: any
suggested: any
priority: normal
tier: 3
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

### 2026-11-01 gpt
Done: Expanded the sharp font-101 visibility mask through the logical line box, bounded by the owning window and its occluders. Added a unit assertion for the expanded area.
Next: Reproduce the exact clipped owner dialogue state and confirm a visible before/after change before setting this task done. The Kcsulik fixture opened a real game dialogue through `gameDialogEnter`, but its old and new screenshots were pixel-identical, so it does not reproduce the reported cut.
Check: Release build passed; ctest 18/18; `tools/regression.ps1` passed 16 spots with 0 changed pixels. The isolated dialogue fixture rendered 10 runs and 295 glyphs. English and Russian message/log and character fixtures rendered at 1920x1080, scale 2. Compact evidence: `runs/140-dialogue-letters-cut/`.

### 2026-11-03 gpt hover-state check

Done: followed the owner crop's yellow hovered-reply state with the existing
`-DialogCursorCheck` path in an isolated worktree. The Kcsulik fixture reached
the 56-byte reply and moved the cursor onto its text, but the sharp-overlay
assertion failed after the move (`opaque_alpha=103->0`, `transparent_alpha=175->195`,
ink pixels `465->439`). The run logs are in
`runs/140-dialogue-letters-cut/dialogue-hover-cursor-check/`. This run produced
no screenshot and does not prove the supplied clipping is reproduced. It is the
second distinct dialogue-state attempt after the normal Kcsulik fixture above.
PR #206 stays draft; task remains `doing` under the two-attempt exception.
Next: continue to task 141. Resume 140 when the hovered reply can be captured
visually or the owner's exact conversation state can be reproduced.
Check: a fresh Release build passed; CTest passed 18/18; the Klamath scene was
exported to the verification worktree's `build/scene-cache`. The prior normal
fixture's before/after remained pixel-identical.
