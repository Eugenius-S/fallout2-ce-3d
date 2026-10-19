---
id: 029
title: The board treats a task with an unfinished dependency as free
milestone: M0
status: todo
owner: none
suggested: glm
claude-only: no
user-check: no
depends-on: [008]
reviewed: no
---

## Goal

`tools/tasks.ps1 -Next` never offers a task whose dependencies are not all
`done`.

## Context

- In `-Next`, the free-task filter maps each dependency to `$done[id]` and
  keeps the task when the result `-notcontains $false`. A dependency that is
  not done gives `$null`, not `$false`, so every dependency passes: 021
  (depends on 013 and 020, 020 is `doing`) counts as free.
- The "nothing is free" branch already finds missing dependencies correctly
  (`!$done[...]`); the free filter can use the same test.

## Acceptance

- A task with any dependency not `done` is not offered by `-Next`; it is
  listed by the "nothing is free" report with what it waits for.
- Checked by hand on the current board: with 020 `doing`, 021 is not free
  for any agent; the output is written into the `Log`.

## Out of scope

Other changes to the board.

## Log
