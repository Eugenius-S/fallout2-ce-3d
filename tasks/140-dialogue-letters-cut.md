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

### 2026-11-03 gpt: sanitize failed-run log

Done: removed the local scene-cache path from the failed cursor-check stdout log. The PR's last CI failure was the public-content check on that path; the screenshot attempt remains a failed assertion and no additional runtime attempt was made.
Next: retain the draft until the clipped reply is reproducible and the sharp overlay passes with matched before/after evidence.
Check: `tools/check_public_content.py origin/main` and `tools/check_runs_size.py origin/main` pass after this commit.

### 2026-11-04 gpt: merge fresh origin/main

Merged `origin/main` at `3b288fd6` with no conflicts. Existing normal and hover-state fixtures still do not reproduce the owner's clipped reply; the hover check assertion failed and did not save a frame. PR #206 stays draft pending a visible matching reproduction.

### 2026-11-04 gpt: current main merge and hover assertion diagnosis

Fetched and merged `origin/main` at `8e135a73867b` without conflicts. Checked
the last hover-state log: the opaque cursor pixel correctly covers underlying
glyph alpha (`103 -> 0`), the transparent cursor cell still contains ink
(`175 -> 195`), and reply ink remains (`465 -> 439`). The assertion required
the transparent cell's alpha to be numerically unchanged, even though hovering
changes the reply highlight. Updated the fixture to require visible ink at
that transparent cell before and after the move, while retaining the opaque
cursor, same-reply, and nonzero-ink checks. This is a test-only diagnostic
change; no image was produced and the clipped reply is still unreproduced.

The updated source compiled, but the local Release link failed with unresolved
OpenGL query-pointer symbols referenced from `perf.cc`; the development
executable was not rebuilt, so no third hover capture was run. CTest reported
19/20, with `settings_3d_tests` failing its `clean_edges` default assertion.
The PowerShell parser passed. PR #206 remains draft and task 140 stays
`doing`; next, resolve the build blockers and capture the actual hovered reply
before claiming visual acceptance.

### 2026-11-04 gpt: hover-capture diagnosis after rebuilding

Rebuilt Release successfully with `tools/configure-build.ps1`; CTest passes
20/20. Added a missing `tools/check_png_pixel.py` helper used by the snapshot
validator and corrected the cursor check to allow hover-highlight alpha to
change while requiring glyph ink under the transparent cursor cell. The
earlier direct CMake build invocation lacked the project's MSVC include
environment and failed before linking; it was not used for runtime validation.

The third capture reached `DIALOG_CURSOR_CHECK PASS`, but review of its image
showed that the cursor was over the NPC message, not an answer. The fourth
capture failed because the selected answer text was not present in the screen
buffer. A fifth diagnostic capture confirmed the fixture was still on the NPC
message (`You have entered a small town called Klamath.`); the answer window
records existed separately and had no matching screen-buffer owner records.
This does not reproduce the owner's clipped answer and provides no accepted
hover evidence. Two consecutive attempts failed the required answer-hover
capture, so the runtime check is blocked pending a fixture that reaches the
actual answer state. Diagnostic source changes are unverified and PR #206
remains draft. Release build and CTest 20/20 pass; no hover acceptance is
claimed.

### 2026-11-04 gpt: release after answer-capture limit
Done: rebased the existing branch on current `origin/main` (`f2ef1236`). The two latest answer-hover attempts still do not expose the selected reply in the screen buffer, so there is no valid after-frame for the owner's clipping report. The older dialog, message and character captures do not reproduce that reported state. Task 140 remains incomplete; the claim is released, and PR #206 stays draft.
Next: resume only when the dialogue fixture can expose and capture the actual selected answer text, then produce the required matched English and Russian crops.
Check: the last Release build and CTest passed 20/20 before rebase. The rebased PR CI will validate the current SHA; no more runtime retries are made under the two-failure limit.
