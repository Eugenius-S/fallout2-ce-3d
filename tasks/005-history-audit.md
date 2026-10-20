---
id: 005
title: Audit the history before going public
milestone: M0
status: done
owner: claude
suggested: gpt
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

Nothing in the full Git history should stop the repository from being public.

## Context

The repository is private now (`.foundation/project.yml`: `visibility:
private`). Game files are ignored by `.gitignore`, but the history has ~2370
commits, including a merge of upstream CE and earlier experiments on
`archive/*` branches.

## What to check, on all branches and all history

- Secrets: tokens, keys, passwords (use a scanner such as gitleaks, plus a
  manual look at its hits).
- Game files: `*.dat`, `*.frm`, `*.map`, `*.pro`, `*.acm`, `*.msg`, `*.sav`,
  `*.aaf`, `*.fon`, palettes, extracted game art other than screenshots.
- Large blobs (the 30 biggest objects) and what they are.
- Third-party code and its licenses: everything vendored is listed with its
  license (`tools/scene_export/VENDORED.md`, `NOTICE.md`).

## Acceptance

- A report in this task's `Log`: what was found, where, and a proposed fix for
  each item.
- History is rewritten only if the owner agrees, in writing, in the chat.
- After the owner makes the repository public: `.foundation/project.yml` says
  `visibility: public`.

## Out of scope

Flipping the visibility: the owner does it.

## Log

### 2026-10-19 claude
Done: audit of all branches on GitHub (full mirror clone; the upstream CE
history it contains is already public and was left out: 88 own commits).
- Secrets: none (gitleaks over the 88 commits, plus a grep for token
  patterns).
- Game files: `main` is clean. Branch `archive/dlss5`, commit `4c0bda8d`
  ("Init: Fallout 2 DLSS5 upgrade kit project"), holds parts of a game
  install: `FALLOUT2_EV.exe`, `ddraw.dll`, `SF-Configurator.exe`,
  `uninstall/unins000.exe`, and `!docs/Fallout2_Manual.pdf`,
  `!docs/Fallout_Bible.pdf`, `!docs/RP_Walkthrough_(RU).pdf`.
  Fix: save the `archive/*` branches as a bundle outside the repository and
  delete `archive/dlss5` from GitHub before going public.
- Personal data: the owner's name and personal e-mail are the author of the
  two squash merges made on GitHub (#1, #2), because GitHub uses the
  account's e-mail. Fix: GitHub settings, "Keep my email addresses private"
  (new merges then use the noreply address); the two commits on `main` stay
  (rewriting `main` would break every agent's checkout). Windows user paths
  (`C:\Users\...`) appear only on `archive/dlss5` and `archive/native-dlss5`.
- Large files on `main`: screenshots in `runs/` up to 7 MB each; regression
  references about 2 MB each. Acceptable.
- Licenses: `ce/` is under the Sustainable Use License 1.0
  (`ce/LICENSE.md`); each library in `ce/third_party/` has its license; the
  exporter and its vendored code carry `LICENSE`, `NOTICE.md`,
  `VENDORED.md`. There is no license file at the repository root, so GitHub
  will show "no license". Fix: a root `LICENSE.md` with the same Sustainable
  Use License 1.0 as CE (the owner decides).
Next: the owner decides on the three fixes; then make the repository public
and set `visibility: public` in `.foundation/project.yml`.
Check: this report.

### 2026-10-19 claude
Done, as the owner decided: the `archive/*` branches are saved as a verified
bundle outside the repository
(`E:\Projects\fallout2-ce-3d-archive\archive-branches-2026-10-19.bundle`);
`archive/dlss5` is deleted from GitHub; `LICENSE.md` at the root is the
Sustainable Use License 1.0, the same text as `ce/LICENSE.md`.
Next: the owner turns on "Keep my email addresses private"
(<https://github.com/settings/emails>; the gh token has no `user` scope) and
makes the repository public; then set `visibility: public` in
`.foundation/project.yml` and close this task.
Check: `git ls-remote origin` shows no `archive/dlss5`.

### 2026-10-19 claude
Done: the owner hid the e-mail; the repository is public
(<https://github.com/Eugenius-S/fallout2-ce-3d>); `.foundation/project.yml`
says `visibility: public`.
Check: `gh repo view Eugenius-S/fallout2-ce-3d --json visibility`.
