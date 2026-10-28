---
id: 106
title: The public repository holds only the project, no personal setup or paths
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The repository is public. A visitor sees the game, its tools, its docs and
its evidence, and nothing of the owner's own machine: no personal agent
setup, no user names, no local paths.

## Context

- `.foundation/project.yml` is the owner's local agent-system marker
  (`scope`, `source_mode`, `visibility`). Only task 005 ever read it. The
  owner's tooling still looks for it in the checkout, so it stays on disk but
  leaves git.
- `.codex/config.toml` lists the owner's personal MCP servers to switch them
  off. That is private setup, not the project's.
- Found on main, 2026-10-27:
  - `runs/017-options-menu-3d-panel/owner-brief.md` line 7: a path with the
    owner's Windows user name;
  - `runs/091-launcher-updates/run1-install.log`, `run2-update.log`,
  `run4-update-c091.log`: local checkout paths;
  - `runs/010-scene-per-map/tour2/devhero-20261019-124706-000.jpg`: the byte
    match is in the encoded image data; the file has no personal metadata.
- `tools/` top level has one-off scripts from past tasks
  (`round5_sheet.py`, `muzzle_sheets.py`, `effect_*`, `glow_ladder_sheets.py`
  and the like).
- `runs/` weighs 322 MB on main. History is not rewritten here.

## Acceptance

- `.foundation/` and `.codex/` are out of git (`git rm --cached`) and in
  `.gitignore`; the files stay in the owner's checkout.
- `git grep -il` on main finds no owner's Windows user name, drive path,
  personal account identifier or unapproved e-mail. Text files get the path replaced with a
  neutral one (`<game>`, `<worktree>`); an image with personal metadata is
  re-saved without it. The grep commands and their empty output go in the
  Log.
- A check in CI (next to `tools/check_runs_size.py`) fails a PR that adds a
  personal name, drive path, account identifier or unapproved e-mail.
- One-off scripts in `tools/` that nothing on main runs or names (no other
  script, doc, workflow or open task's `Check`) are removed; the list goes
  in the Log. A script still named somewhere stays.
- Build, tests and regression pass; nothing in `runs/` beyond the files above
  changes.

## Out of scope

Rewriting git history (the 322 MB and old commits stay; the owner decides
that separately), slimming `runs/`, `AGENTS.md` and `CLAUDE.md` (they are
how the project is worked on and stay public).

## Log
