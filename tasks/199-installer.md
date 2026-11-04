---
id: 199
title: A proper installer with the project logo, one-click updates, Russian and RP options
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: high
tier: 1
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

The player installs the game through a real installer window in the
game's look, with the project logo; updates come from GitHub by
themselves or in one click; Russian is an install option, and the
Restoration Project is shown as one, disabled for now.

## Context

- High priority (the owner, 2026-11-04); the installer in Russian and
  English too. It goes right after 184 (RP
  install and boot): the RP option needs to know how RP installs. If gpt is
  busy, glm may take it between map areas.
- The owner, 2026-11-04: a proper installer that looks good, with the
  project logo; updates pulled from git by themselves; options for the
  Russian translation and the Restoration Project install, the latter
  inactive for now.
- Stands alone: builds on 091 (done, `fallout2-ce-3d-launcher.exe`: finds
  Steam or GOG, installs from the rolling `latest` release, builds the
  scene data, checks GitHub for updates on every start). Do not write a
  second updater; extend 091's.
- The README's Plans line "A full installer" (both READMEs) is this task.
- The logo: `docs/images/logo.png` (500x243 RGBA); the owner's sources are
  `logo_final.png` and `logo_final.psd` (in `marketing/logo` after 198).
- Russian today: the player drops a translation into the game folder before
  install. In our own setup it is `rpu_russian.dat` (text and fonts) and
  `upu_russian_sound.dat` (speech) in `mods_order.txt`.
- The Restoration Project is 184-188 (tier 1, next for gpt). The RP option
  turns on when they are done: a note, not a hard block.

## Acceptance

1. The installer window: the project logo, the game's look, an install
   path, a progress bar, a finish page with a Play button.
1a. The installer and launcher speak Russian and English: on first start
   the language follows the system's, a switch in the window changes it,
   and the choice is remembered. The UI language only; the "Russian" game
   option stays a separate checkbox.
2. Updates: the launcher's GitHub check shows what is new (the version and
   short notes), updates in one click or by itself on start (a setting),
   and can roll back to the previous build.
3. Install options:
   - "Russian": the source and its rights settled first, in the Log; then
     it installs a translation whose license allows redistribution, or
     fetches it from its official download, and sets the engine to Russian
     (`fallout2.cfg` language and the `mods_order.txt` entries).
   - "Restoration Project": shown, disabled, with the hint "coming soon".
4. The installer, its options and the Install section of both READMEs
   change together; the Plans line "A full installer" goes when it lands.
5. A clean install on a scratch folder, an update and a roll-back, shot by
   shot, in `runs/199-installer/`.

## Out of scope

RP itself (184-188), macOS.

## Log

### 2026-11-05 gpt
Done: Added the bilingual Win32 installer window and embedded project logo,
release version and notes, game folder detection, install progress, play,
automatic updates, rollback, the official RPU Russian text option, and a
disabled Restoration Project option. Updated both Install sections and removed
the full-installer Plans entries. Evidence is in `runs/199-installer/`.
Rights: the upstream RPU repository has no GitHub license metadata or root
`LICENSE` file, so this change does not assume redistribution rights. The
launcher fetches the translation from the official RPU release at the player's
request and does not bundle or rehost it. RPU project credits list Russian
translators. The UPU translation notes identify Russian text as based on 1C
and Drobovik & Co., but do not license the RPU asset.
Check: MSVC Release build, CTest 20/20, standalone exporter build, clean
install, update-on-launch, rollback, English and Russian UI, and the official
RPU translation install all passed on isolated scratch copies. See the live
screenshots and metadata in `runs/199-installer/`.
Next: CI for PR #285 on the final commit.
