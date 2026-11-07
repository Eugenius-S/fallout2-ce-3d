---
id: 211
title: The 3D mode toggle responds when it can, and says why when it cannot
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

The owner, 2026-11-07: "the 3D mode button does not work." Toggling the 3D
mode in play must either turn the scene on or say plainly why it did not.

## Context

- The only in-game 3D toggle is the settings row "Enable 3D scene"
  (Включить 3D-сцену); there is no interface-bar button. The row flips
  [3d] enabled, and scene_view syncSettings enables the scene on the next
  frame only when a real map is loaded (not the main menu, not a
  transition).
- enable() refuses with "3D scene is disabled in settings" when enabled is
  off, and showsCurrentMap() gates the enable: a map without a 3D scene
  (no exported scene data) stays 2D silently. A refusal on a sceneless map
  reads to the owner as "the button does not work".
- The owner played on their current save; which map they toggled on is
  unknown, so the exact refusal is unconfirmed.

## Fix

- When the toggle cannot enable the scene, surface the reason in the
  game's status line in Russian (which map has no scene, what failed).
- Check whether the owner's failing case is a sceneless map and whether
  the map list of sceneless locations is worth showing in the toggle's
  description.

## Acceptance

- On a map with a scene the toggle turns the 3D view on and off.
- On a map without one the toggle says so, in Russian, in the game.

## Log

- 2026-11-07: owner report; the toggle path and its silent gates found the
  same day; the reason surfacing is pending.
