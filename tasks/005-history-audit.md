---
id: 005
title: Audit the history before going public
milestone: M0
status: doing
owner: claude
suggested: gpt
claude-only: no
user-check: yes
depends-on: []
reviewed: no
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
