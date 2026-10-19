---
id: 017
title: Options menu with a 3D panel
milestone: M1
status: todo
owner: none
suggested: gpt
claude-only: no
user-check: yes
depends-on: [014]
reviewed: no
---

## Goal

Every new setting is in the game's own options menu.

## Context

- The game's preferences window is in `ce/src/options.cc`.
- The `[3d]` settings and their descriptors come from task 014; each feature
  task adds its keys.
- The owner's picture: the preferences window scrolls; below the original
  settings there is a large panel styled like the Pip-Boy screen, listing all
  new options in a regular readable font.

## Acceptance

- The preferences window scrolls by wheel, by a scroll bar and by keyboard; the
  original settings look and work as before.
- The panel lists every `[3d]` descriptor in order with its label and current
  value: switch, slider or choice by type. Changes apply at once where the
  descriptor says so, otherwise after the menu closes; they save to
  `fallout2.cfg`.
- New keys appear without menu code changes.
- Works in 2D and 3D, at x2 and x3 scale, in Russian and English game
  languages.
- Snapshots of the window in the PR.

## Out of scope

The sharp text layer (task 025); the panel uses the game's text drawing now.

## Log
