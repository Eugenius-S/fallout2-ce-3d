---
id: 197
title: A trial AI HD remaster of one character's standing and walking sprites
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: low
tier: 4
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A trial, nothing goes into the game: one character's sprites redrawn at x2
and x4 by an image model, to see whether it keeps the original's look and
holds steady from frame to frame.

## Context

- The owner, 2026-11-04, with a news shot about the "FFRS HD Sprite
  Remaster" mod (849 characters and 20,500 frames redrawn with AI at x2 and
  x4; some players felt it lost the original's soul):
  `runs/inbox/2026-11-04-hd-sprite-remaster-reference.png`. Try the same
  on one character, not urgent.
- Stands alone: no open task is an art pipeline experiment.
- One character, for example the player's default look.
- The senior model writes the generation prompt from the original frames:
  what to keep (the palette, the silhouette, the proportions, the light
  direction, the pixel-art feel) and what to sharpen.
- The generation runs on Luna light (`gpt-6-luna`, low effort) with that
  prompt: the standing frame in all 6 directions, and every frame of the
  walk or run cycle in one direction.

## Acceptance

In `runs/197-hd-sprite-trial/`, never in game data:

- An original vs generated sheet at x1, x2 and x4.
- The prompts used.
- A note on frame-to-frame consistency (the animation must not flicker),
  on alignment with the original frame offsets, and on cost and time per
  frame.

## Out of scope

Putting any of it into the game; other characters.

## Log
