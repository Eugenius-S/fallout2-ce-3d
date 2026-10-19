---
id: 008
title: The board tells each agent what to do next
milestone: M0
status: doing
owner: glm
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

### 2026-10-19 glm
Done: tools/tasks.ps1 gained -Next <agent> (its own doing task first, then
the free task by the AGENTS.md rule with a blocked-tasks report) and -Review
(done tasks with reviewed: no); both read tasks from origin/main after a
git fetch and fall back to the working tree with a note. AGENTS.md uses
-Next in "Taking a task" step 3 and -Review in the Review section.
Checked by hand on the current board:
- -Next claude -> tasks/018-see-through-walls.md, branch
  task/018-see-through-walls (its doing task);
- -Next gpt -> tasks/027-3d-settings-per-key.md, branch
  task/027-3d-settings-per-key (its doing task);
- -Next glm -> tasks/008-task-board-next.md (this task);
- -Review -> 001, 002, 005, 010, 011, 013, 024.
Next: merge; the PR run of the Verify workflow is expected green (no game
code touched).
Check: the three -Next commands and -Review above.
