---
id: 215
title: The build's hotkeys work like a set, and F1 lists them
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

The owner, 2026-11-07: "let's make the hotkeys human." The set:

- Ctrl+F10 — switch original / reimagined view (2D classic ↔ 3D scene).
- Ctrl+F11 — 3D fly (replaces the old aspect-switch restart binding).
- F12 — screenshot; Ctrl+F12 — screenshot with the highlight.
- F9 — the FPS counter, if the game does not already use F9 (it does: the
  feedback shot from 183 sits there; the counter stays on F8 until the
  owner says to move it).
- The F1 help screen lists all the build's new hotkeys at the bottom.

## Context

- kb.cc maps scancodes to logical keys per modifier (unmodified, shift,
  alt, ctrl) — the ctrl slots exist for the F-row.
- The view toggle glue exists: scene::enable(View::Full) / disable() /
  activeView(); the refusal reasons from task 211 must surface in Russian
  here too.
- Fly mode lives behind the dev tools today (landFlight / stopFlying);
  the hotkey path needs a player-safe start/stop without dev flags.
- The highlight for Ctrl+F12: the game's mouse highlight of the object
  under the cursor; the exact composition is pinned in task 213.

## Acceptance

- All four key bindings work in a plain launch (no dev flags).
- F1's help screen ends with the build's hotkey list, in Russian with the
  game's language.
- Every binding answers with a Russian status line when it cannot act.

## Log

- 2026-11-07: owner order with the exact binding table; F9 found occupied
  by the feedback shot (183).
