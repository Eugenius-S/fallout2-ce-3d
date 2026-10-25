---
id: 079
title: A lighter repository and fewer tokens spent on reading
milestone: M1
status: done
owner: claude
suggested: claude
priority: high
claude-only: no
user-check: no
depends-on: []
reviewed: yes
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
### 2026-10-25 claude
Done: measured from the agents' own session records, then cut.
- Where the tokens go: re-reading the thread. Claude's six sessions here
  spent 3.9 billion tokens, 99% of them cache reads; a call re-read 330k
  tokens at the median (78k at a session's first call, up to 959k), while
  all command output together was under 1% (3.1M). gpt's nine sessions:
  856M input tokens, 98% cached, 121k per call at the median over 7104
  calls. So the cost is calls times thread length, not big outputs.
- Polling for the test slot was about 10% of gpt's calls (149 of 1417 on
  2026-10-24), each re-reading ~120k: `dev-snapshot.ps1` now waits for the
  slot itself (a named mutex, then the old process check; `-WaitMinutes`,
  30 by default). Two snapshots started together ran one after the other,
  both exit 0.
- AGENTS.md: the cost model in one rule (one task per thread, a new thread
  once it is long, few calls, no polling, images cropped and one at a
  time), and evidence in `runs/` of at most 1 MB per file.
- Repository: the 97 PNGs over 1 MB in `runs/` are JPEGs now (173.9 MB ->
  58.3 MB; `runs/` 356 MB -> 241 MB in a checkout), the references in 49
  notes follow; the PNGs stay in history. CI: a `runs-size` job refuses an
  added or changed `runs/` file over 1 MB (`tools/check_runs_size.py`;
  it caught 071's and 072's own sheets against an older base), and every
  other job checks out without `runs/`.
- Git noise: `core.safecrlf false` in the shared repo config ends the "LF
  will be replaced by CRLF" lines. The proxy probe warnings come from the
  credential helper's settings on this machine: the owner's to change.
- History is still 981 MB of packs: the rewrite stays with 005.
Next: none. The owner's side: fewer connectors in this project (the
first call already carries 78k tokens), and threads restarted per task.
Check: `py -3 tools/check_runs_size.py origin/main`; two parallel
`dev-snapshot.ps1` runs both succeed.
