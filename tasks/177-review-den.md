---
id: 177
title: The owner's fly-through of the Den, turned into fix tasks
milestone: M1
status: todo
owner: owner
suggested: any
priority: low
tier: 4
claude-only: no
user-check: no
depends-on: [183]
reviewed: no
---

## Goal

The owner flies through the Den (task 158, the `den` map group), presses
the screenshot key (183) on every defect and writes a note. An agent from
the pool then turns the shots and notes into fixes.

## Context

- The owner, 2026-11-04: the per-city review is the owner's job; agents miss
  the fine visual defects the owner's eye catches.
- The owner's part: shots with their JSON and a notes file in
  `runs/177-review-den/`. When it lands, the owner sets `owner: any`.
- The agent's part, after that: read the batch the way `docs/owner-shots.md`
  says (183), match each note to its shot, and file every find into the
  batch of its kind (189 walls, 190 roofs, 191 furniture and props, 192
  vehicles, 173 what cannot be built, the UI batch), or fix it here when it
  belongs to this city alone. One PR per batch run.

## Acceptance

- Every note in the Log, marked with the task or PR it went to.

## Out of scope

The flight itself (the owner's).

## Log
