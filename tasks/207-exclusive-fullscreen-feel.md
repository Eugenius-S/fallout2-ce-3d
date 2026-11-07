---
id: 207
title: The owner gets an exclusive-fullscreen feel from the borderless setup
milestone: M1
status: open
owner: any
suggested: owner
priority: medium
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The owner, 2026-11-07: "borderless mode does not work, the game is always
fullscreen and takes very long to minimize." Make the display mode the owner
picks be the mode the game shows, with a fast minimize.

## Context

- The owner's `game/fallout2.cfg` says `windowed=1` (plain window); the demo
  copy says `windowed=2` (borderless). Which copy and which menu pick the
  report is about needs one owner answer.
- Checked on the demo copy with the current main build (990d7756), both
  modes do what the code says: `windowed=1` makes a normal window
  (WS_CAPTION styles present), `windowed=2` makes a popup window covering
  the whole 3440x1440 desktop (SDL_WINDOW_FULLSCREEN_DESKTOP). Neither is
  exclusive fullscreen.
- "Very long to minimize" is the classic exclusive-fullscreen symptom;
  SDL_WINDOW_FULLSCREEN_DESKTOP with an OpenGL context can also minimize
  slowly on some drivers. The window style of the owner's live game process
  settles which one it is (`owner_window_style.ps1` in the diag folder reads
  it without touching the game).
- The mode selector writes through the standard `windowed` setting clamp
  (Fullscreen 0, Window 1, Borderless 2), so a wrong write is unlikely.

## Acceptance

- With the owner reproducing their exact steps, the window style matches the
  picked mode, and minimizing takes no longer than other borderless games.

## Out of scope

- Resolution and DPI questions that are not about the mode itself.

## Log

- 2026-11-07: owner report; both modes verified sane on the demo copy the
  same hour; waiting for the owner's one-line repro (which copy, which pick).
- 2026-11-07, later: the owner's plain launch narrows the report. Their cfg
  runs windowed mode with opening movies skipped; the window style read from
  a live run carries the caption and the minimize box (no exclusive
  fullscreen at these settings). The white one-pixel strip they reported
  turned out to be the Windows 11 DWM border line; the fix and the
  slow-minimize guidance live in task 208. Exclusive fullscreen at a
  non-native resolution remains the known slow-minimize case if the report
  comes back.
