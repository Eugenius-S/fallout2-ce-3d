---
id: 066
title: The dialogue screen hides the street behind black
milestone: M1
status: doing
owner: gpt
suggested: glm
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

While a dialogue is open, the free screen area around the dialogue window is
black, so the street world does not show around it.

## Context

- The owner's find of 2026-10-24, with their reference:
  `runs/inbox/2026-10-24-dialog-black-background.png`.
- On a wide screen the dialogue window covers only the middle; the 3D scene
  keeps drawing around it.

## Acceptance

- Snapshots of one dialogue at the owner's resolution and at 1920x1080 under
  `runs/066-dialog-black-background/`: everything outside the dialogue
  window is black, the window itself unchanged.
- Leaving the dialogue brings the street back the same frame.

## Out of scope

The dialogue's own text and layout.

## Log
