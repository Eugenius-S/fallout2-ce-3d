---
id: 001
title: Process files for three agents
milestone: M0
status: done
owner: claude
suggested: claude
claude-only: yes
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

Any agent can take, continue and hand off work without asking anybody.

## Context

Decided with the owner on 2026-10-19: see `docs/2026-10-19-roadmap-design.md`.

## Acceptance

- `AGENTS.md` describes the rules, build and checks, the task format and the
  task cycle; `CLAUDE.md` points to it.
- `tools/tasks.ps1` prints the board.
- `.github/pull_request_template.md` exists.
- Task files for M0 and M1 are written.

## Out of scope

Translating the existing Russian docs (task 006).

## Log

### 2026-10-19 claude
Done: roadmap, `AGENTS.md`, `CLAUDE.md`, PR template, `tools/tasks.ps1`,
tasks 001-025.
Next: the owner reads the roadmap and the tasks.
Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/tasks.ps1`.
