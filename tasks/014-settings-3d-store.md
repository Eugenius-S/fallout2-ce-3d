---
id: 014
title: Settings store for 3D features
milestone: M1
status: doing
owner: gpt
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

### 2026-10-19 gpt
Done: Claimed task 014 to add the schema-backed `[3d]` setting store and a small scene API for reading values and detecting updates.
Next: Inspect the current settings schema and scene APIs, then propose the smallest compatible design before implementation.
Check: `tools/tasks.ps1`; task branch `task/014-settings-3d-store`.

### 2026-10-19 gpt
Done: Registered `[3d].enabled` with default on, strict key-specific boolean parsing, fallback menu metadata, and a scene API exposing the value plus a non-consuming monotonic revision. Added real-registry tests and three-line key-add guidance.
Next: Parent review of Step 1; scene activation remains for Step 2.
Check: Release build; root CTest 8/8; standalone `settings_3d_tests` 1/1.

### 2026-10-19 gpt
Done: Connected `[3d].enabled` to scene lifecycle updates: enabled defaults to Full gameplay scenes, disabled gates all 3D views, and revision changes switch immediately. Ctrl+F10 enables the schema setting before switching Full. Startup snapshots wait for target map/elevation readiness; vanilla presentation supplies a one-shot when 3D is off or unavailable. Dev snapshots now support `-NoPreview`; pending map tours also run startup positioning.
Next: Parent review of Step 2; full regression and task completion remain out of scope.
Check: Release build; CTest 8/8; standalone settings suite 1/1. Background snapshots through `tools/dev-snapshot.ps1` on EXE SHA-256 `a5d24a31f7b3a9882bfcce5001ea8ccb8947cc5183e30af630f5dc6dda32f7da`: missing/default, `enabled=1`, invalid `enabled=2` yielded enabled Full; `enabled=0` yielded disabled vanilla framebuffer. All at KLARATCV e1 tile 14701, 1920x1080/logical 960x540/scale 2/windowed. ARTEMPLE e0 same-map `-NoPreview` fallback captured tile 17288; no-preview KLADWTWN0 → KLARATCV1 → KLADWTWN0 tour completed with requested metadata. Fixture config hashes remained unchanged; owner config remains `875111504363B0A54B63002919598B175E64DECF9E84C5F5A67BC915291FE927`. Evidence is under ignored `build/task014-proof/`; no full regression run.
