---
id: 209
title: Display changes apply without a restart
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

The owner, 2026-11-07: "I set 3440x1440 and borderless, pressed Done, and
nothing changed; the resolution only changed after a restart. Resolution
and scale changes must happen on the fly, without restarting the game."

## State

- The window mode (Fullscreen, Window, Borderless) now applies live: the
  moment the preferences panel closes with Done, the window switches in
  place (svga applyWindowModeLive from the save path); the panel row no
  longer says it needs a restart.
- The resolution and the interface scale still need a restart. Their
  change resizes the engine's screen surface, and every interface window
  sizes itself from the screen at creation; a live path means rebuilding
  the whole screen subsystem mid-game, which is its own task slice.

## Acceptance

- Picking a display mode applies it immediately, verified by a live
  window-style read.
- Resolution and scale either apply live or the restart note is the only
  thing the panel promises, with the honest scope recorded here.

## Log

- 2026-11-07: owner order; window mode shipped live the same day,
  resolution and scale stay on the restart note until the screen-subsystem
  rebuild slice.
