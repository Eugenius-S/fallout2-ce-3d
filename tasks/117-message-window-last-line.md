---
id: 117
title: The message window's last line shows whole
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The bottom line of the message window under the interface bar shows in
full, not cut at its foot.

## Context

- The owner, 2026-10-28, live on main `ff5f4ed0`, Russian text:
  `runs/inbox/2026-10-28-chat-bottom-line-cut.png`. The lowest line's
  letters lose a pixel or two at the bottom.
- Check whether the original game cuts it the same way with this font
  (the Russian font's descent against the window's line height) before
  changing anything; if it is ours (the text layer or the 3D build's
  scaling), fix ours.

## Acceptance

- before | after of the message window full of lines, in
  `runs/117-message-window-last-line/`, at 16:9 and 4:3.
- The original view regression stays pixel for pixel unless the cut is
  ours there too; ctest passes.

## Out of scope

The message window's art, font choice, line count and text wrapping.

## Log
