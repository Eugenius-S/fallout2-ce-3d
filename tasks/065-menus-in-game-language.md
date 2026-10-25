---
id: 065
title: Our menus speak the game's language
milestone: M1
status: doing
owner: gpt
suggested: glm
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

With the game set to Russian, the menus we added (the [3d] options panel,
its labels and descriptions, the dev panel's player-facing lines) show
Russian text, and English with the game in English.

## Context

- The owner's find of 2026-10-24: the custom menus keep English labels
  while the game runs in Russian.
- The labels come from `ce/src/settings.cc` (`fallbackLabel`,
  `fallbackDescription`); the game's own strings come from its message
  files by the active language. Pick one place for our strings per
  language, with the English line as the fallback when a translation is
  missing.

## Acceptance

- Snapshots of the options panel in Russian and in English under
  `runs/065-menus-in-game-language/`.
- Every [3d] key has a Russian label and description; a test fails when a
  new key lands without one.

## Out of scope

The font itself (057).

## Log
