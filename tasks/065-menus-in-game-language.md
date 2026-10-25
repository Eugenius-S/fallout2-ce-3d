---
id: 065
title: Our menus speak the game's language
milestone: M1
status: done
owner: gpt
suggested: glm
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
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

### 2026-10-25 gpt

Done: shared English/Russian menu catalog in `ce/src/scene/menu_text.*`.
Settings initialization translates all 13 `[3d]` labels and descriptions;
the options title, section headings, toggles and developer panel follow the
game language. Missing strings and other languages retain English. The
catalog supplies CP1251 to the existing fonts without changing font assets.
Added a coverage gate for future `[3d]` keys and real Russian/English settings
initialization tests. Added background developer-panel capture support and
fixed the helper's existing vanilla PNG/sidecar location mismatch. Screenshots
and verification metadata are in `runs/065-menus-in-game-language/`.

Next: no remaining work in 065. The owner's dialogue hanging-indent request
is recorded in `tasks/inbox.md` as backlog only, with no layout implementation.

Check: Release build; CTest 15/15; exporter tests 52/52; regression unit tests
12/12. All 16 world scenes match the current-base controls from 066 with 0
changed pixels. This uses the same exported scene cache and temporary control
references from that task; the old committed references were not changed
(their drift remains task 084). Runtime captures show both languages in the
options and developer panels. The normal helper's `-DevPanel` path was tested
without focusing or changing the owner's running game.
