---
id: 027
title: 3D settings report changes per key and log a failed start
milestone: M1
status: todo
owner: none
suggested: gpt
claude-only: no
user-check: no
depends-on: [014]
reviewed: no
---

## Goal

Each 3D feature reacts only to its own setting, and a feature that fails to
start says so in the log, as `AGENTS.md` asks.

## Context

From the review of task 014. `settingsGetThreeDRevision()` counts changes of
the whole `[3d]` section; `scene::syncSettings` turns the full view on or off
at any new revision. With a second key (light, task 020) changing it would
switch the scene on again even if the player had it off. `syncSettings`
drops the error from `scene::enable`.

## Acceptance

- A feature learns about changes of its own key only (for example a revision
  per key, or the consumer keeps the last value it saw).
- When `[3d] enabled` asks for the scene and it fails to start, one log line
  says why, and the game stays in 2D.
- Unit test: changing another `[3d]` key does not change what the scene
  lifecycle does.

## Out of scope

The options menu (017).

## Log
