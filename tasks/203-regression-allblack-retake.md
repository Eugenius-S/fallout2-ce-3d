---
id: 203
title: The regression harness retakes an all-black capture instead of diffing it
milestone: M1
status: todo
owner: glm
suggested: glm
priority: normal
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

When a spot's capture comes out all black (the known "no snapshot"
flake's sibling: the snapshot lands before the first real present),
the regression harness recognizes it and retakes the capture once
before comparing, so a gate does not fail a spot with a 2-megapixel
diff.

## Context

- Found 2026-11-06 on the 183 control gate: sanfran-tanker-deck
  reported "fail 2073600 changed pixels" (the whole 1920x1080 frame),
  the capture was a fully black frame, the single-spot rerun passed
  with 0 changed pixels. The same flake family as "No snapshot within
  90 s" (that one already retries).
- The check belongs where the capture lands (tools/regression.ps1 or
  the capture step it drives), before check.py compares: a capture
  whose every pixel is black is not a render, it is a missed frame.

## Acceptance

- A synthetic check: feeding the harness a black capture triggers the
  retake (log line names the spot and the retake), a second black
  capture fails the spot with an honest message, a real capture
  compares as before.
- A full gate on main stays green and takes no noticeably longer on
  healthy spots.

## Out of scope

The "No snapshot within 90 s" retry and any other harness behaviour.

## Log
