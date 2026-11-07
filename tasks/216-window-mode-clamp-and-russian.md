---
id: 216
title: The Window mode choice stops collapsing into exclusive fullscreen, and the owner's build speaks Russian
milestone: M1
status: done
owner: claude
suggested: owner
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The owner, 2026-11-07 14:52, after launching from the "Run F2R" shortcut:
"почему сборка с ярлыка ОПЯТЬ фулскрин и ОПЯТЬ на английском?" — the two
complaints they already made this morning came back.

## Cause

Two independent defects, both invisible in the agent's test runs because the
test config is not the owner's config:

- The settings registry clamps `screen.windowed` to
  `clamp(Fullscreen, WindowedFullscreen)` = 1..2, while the settings window
  offers Window (0) as a choice. The clamp rewrites the value on load and on
  every menu assignment, so a saved Window pick turned into exclusive
  fullscreen on the next launch — the mode that is hard to minimize ("ОПЯТЬ
  фулскрин, сворачивается крайне тяжело").
- The Russian settings catalog (210) only activates when `language=russian`
  in fallout2.cfg. The owner's config said `language=english`, so the merged
  translations never fired on their machine; the agent's test config had
  russian set by hand, which is why the verification screenshots looked
  right while the owner's build stayed English.

## Fix

- `screen.windowed` clamps over the full offered range
  `clamp(Windowed, WindowedFullscreen)`; a saved Window survives the load
  and the menu assignment (unit checks seed windowed=0 and windowed=2
  through the fake config).
- `showSplash` falls back to the stock `art\splash\` when the localized
  `art\<language>\splash\` folder is missing, so a Russian cfg on an install
  without localized splash art still shows a splash instead of none.
- `soundInit` refuses to start under `F2CE_DEV_BACKGROUND`, so every dev
  test boot stays silent no matter which config it borrows — the 25 s smoke
  boot audible to the owner (2026-11-07, the demo copy's sound was on)
  cannot happen again. The game already treats sound-init failure as
  "continue without sound".
- The owner's `fallout2.cfg` flips `language=english` to `language=russian`
  (safe on this install: localized art and font lookups fall back to the
  stock paths when the folders are absent; the fallback splash covers the
  one gap). No code default changes: fresh English installs keep English.

## Verification

- settings_3d test: windowed=0 and windowed=2 survive `initializeWith`.
- The regression sweep from the 13:24 run (139 captured spots
  pixel-identical) is unaffected: the clamp change only widens the accepted
  range, test configs use windowed=1 which clamps identically.
