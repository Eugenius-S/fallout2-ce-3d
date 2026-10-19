---
id: 008
title: The board tells each agent what to do next
milestone: M0
status: todo
owner: none
suggested: glm
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

An agent that starts a session asks the board one question and gets the task
it should work on, so nobody misreads dependencies or takes a claimed task.

## Context

`tools/tasks.ps1` prints every task's header as a table. The rules for taking
a task are in `AGENTS.md`, "Taking a task". The board must be read from
`origin/main` (after `git fetch`), because another agent may be working on a
branch in another worktree.

## Acceptance

- `tools/tasks.ps1 -Next <agent>` prints one task with its file and branch
  name `task/NNN-slug`: the agent's own `doing` task first; otherwise the
  free task by the rule in `AGENTS.md` (suggested for the agent first, then
  any); otherwise says nothing is free and which tasks block the agent.
- `tools/tasks.ps1 -Review` lists `done` tasks with `reviewed: no`.
- Both read the task files from `origin/main` (`git show origin/main:tasks/...`)
  after a `git fetch`; without a network they fall back to the working tree
  and say so.
- `AGENTS.md`, "Taking a task", uses `-Next` instead of describing the rule by
  hand, and the Review section uses `-Review`.
- Checked by hand on the current board: the output for `claude`, `gpt` and
  `glm` is written into the `Log`.

## Out of scope

Changing the task format.

## Log
