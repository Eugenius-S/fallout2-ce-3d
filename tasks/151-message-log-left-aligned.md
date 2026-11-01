---
id: 151
title: The message log lines all start at the same left edge
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
