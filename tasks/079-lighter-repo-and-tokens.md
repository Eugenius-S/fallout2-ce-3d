---
id: 079
title: A lighter repository and fewer tokens spent on reading
milestone: M1
status: doing
owner: claude
suggested: claude
priority: high
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Agents spend their limits on the work, not on reading: the repository
stops growing by megabytes of screenshots per task, and the files every
session reads are short.

## Context

- The owner, 2026-10-25: a priority; gpt runs out of limits very fast.
- Today: `.git` is about 886 MB, `runs/` about 2.1 GB; task files with
  long logs (025 is 39 KB, all tasks 325 KB); git prints proxy warnings
  on every command; agents read screenshots as images.
- Measure first: what a typical session reads (AGENTS.md, the task, its
  logs, command output, images) and where the tokens go; then cut the
  biggest.
- Likely cuts: one JPEG sheet per task in git, everything else outside
  (`game/shots/`) with a size check in CI; done tasks' logs folded to a
  short summary; quiet git output; images downscaled before an agent
  reads them. Rewriting history is the owner's switch (the public audit,
  005).

## Acceptance

- The measurement and the cuts in this task's `Log`, with before and
  after numbers.
- CI refuses a PR that adds a file over the size limit to `runs/`.

## Out of scope

The history rewrite itself.

## Log
