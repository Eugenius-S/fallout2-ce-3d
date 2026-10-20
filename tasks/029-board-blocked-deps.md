---
id: 029
title: The board treats a task with an unfinished dependency as free
milestone: M0
status: done
owner: glm
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

### 2026-10-19 glm
Done: the free-task filter casts each dependency lookup to [bool], so a not
done dependency (which yields $null) counts as unfinished and the task is
not offered. One-line change in tools/tasks.ps1.
Checked by hand on the current board (020 doing):
- -Next claude -> tasks/018-see-through-walls.md (its doing task);
- -Next gpt -> tasks/016-clean-map-edges.md (its doing task);
- -Next glm -> tasks/029-board-blocked-deps.md (this task);
- -Next nobody (no doing task, runs the free path) -> tasks/017
  (options-menu-3d-panel); neither 021 (waits for 020) nor 023 (waits for
  021) is offered.
Next: merge.
Check: tools/tasks.ps1 -Next <agent> with 020 doing - 021/023 are absent
from every output.
