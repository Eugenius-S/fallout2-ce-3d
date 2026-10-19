---
id: 014
title: Settings store for 3D features
milestone: M1
status: todo
owner: none
suggested: glm
claude-only: no
user-check: no
depends-on: [002]
reviewed: no
---

## Goal

One place where every 3D feature keeps its setting, so the menu (task 017)
and each feature read the same value.

## Context

- The engine has a settings schema: `ce/src/settings.h` (`SettingDescriptor`:
  id, section, key, type, default, apply policy, label, choices),
  `options_schema.cc`, `options_settings_model.cc`. Use it; do not invent a
  second mechanism.
- Settings live in `game/fallout2.cfg`. Our section is `[3d]`.
- 3D is currently switched by dev flags and Ctrl+F10 (dev only).

## Acceptance

- A `[3d]` section described by the schema, with one key now:
  `enabled` (bool, default on): off gives plain 2D Fallout 2, on shows the 3D
  scene where a scene exists. Ctrl+F10 stays a dev shortcut.
- A small C++ API in `ce/src/scene/` to read a 3D setting and to learn that it
  changed, so a feature can switch at once.
- How to add a new key is written in `AGENTS.md` in three lines (key,
  default, menu label) next to the existing rule about switches.
- Unit test: the `[3d]` descriptors parse, defaults apply when the key is
  missing, a bad value falls back to the default.

## Out of scope

The menu UI (task 017).

## Log
