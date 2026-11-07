---
id: 208
title: No stray edge strip in the menu, and the window behaves like a window
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

The owner, 2026-11-07, on a plain launch (no dev flags): "in the game menu
there is a white strip one pixel wide on the left", "the game starts
fullscreen and is very hard to minimize". The menu shows no stray edge
strip, and the windowed mode gives a window the owner can minimize without
a fight.

## Context

- The reported white strip itself: the game runs on Windows 11, which draws
  a light one-pixel border around every window. Against the dark menu art
  it reads as a white one-pixel strip at the window edges (left is where
  the menu is darkest). PrintWindow captures bypass the DWM border, so
  repro captures do not show it; reading the attribute back and setting it
  verified the fix path (S_OK on the demo copy).
- Reproduced the menu on the demo copy with the owner's display settings
  (windowed, 1920x1080, scale factor 2.0) via a background run with the
  intro movies skipped (the owner's cfg skips them too). The capture shows
  a dark edge band at the window's left: the 7 px frame plus a 2 px dark
  column inside the client from the fractional window scale.
- Three more mechanisms feed the same defect family:
  - the menu buffer pre-fills with the palette's white (COLOR_FIRST), so
    any pixel the background art does not cover shows white;
  - the vanilla SDL renderer fallback never sets a draw color, and SDL's
    default is white, so its logical-size letterbox bars render white;
  - the GL presentation letterboxes on a near-fit, and a fractional window
    scale (a 125% DPI desktop resizes the requested window behind the
    game's back) leaves a few-pixel gap at the edges.
- The window at these settings measures 1536x864 client for a 1920x1080
  request (DPI virtualization), which reads as an oversized half-window
  rather than a window; the caption carries a minimize box and no maximize
  box (SDL strips both from non-resizable windows).

## Fix

- The game window opts out of the Windows 11 DWM border line
  (DWMWA_BORDER_COLOR = DWMWA_COLOR_NONE), loaded by name so the call
  silently skips where the attribute does not exist. This is the reported
  strip.
- The menu pre-fill becomes black (COLOR_BLACK), so uncovered pixels can
  not glow white.
- The vanilla renderer fallback sets a black draw color before presenting.
- The shared fitRect stretches to fill when the letterbox gap is at most
  12 px per axis (a near-fit), keeping aspect letterboxing for real
  aspect mismatches (for example a 16:9 picture borderless on a 21:9
  screen); all presentation passes share fitRect, so picking stays
  aligned with the picture.
- The windowed window re-adds WS_MINIMIZEBOX after creation as an
  explicit guard, even though the live style already carries it.

## Acceptance

- The DWM border attribute reads back as none on a launched game window.
- The same repro capture shows no dark band inside the client at the
  window's left.
- The menu still letterboxes correctly where the aspect genuinely differs
  (borderless 16:9 on a 21:9 desktop keeps its side bars).
- ctest passes; the scene regression changes only at menu/edge captures.

## Out of scope

- The "starts fullscreen" half of the report: with the owner's current
  cfg (windowed mode) the repro shows a window, not fullscreen. If the
  report repeats, read the live window style with the diag script while
  the game is up. Exclusive fullscreen at a non-native resolution is the
  known slow-minimize case; the display-mode guidance belongs to task
  207.
- Making the exe DPI-aware (the 1920x1080 request landing at a different
  physical size).

## Log

- 2026-11-07: owner report; repro captured on the demo copy the same
  hour; three white/edge mechanisms identified in code and fixed; the
  live window style was read from the repro (caption + minimize box
  present).
