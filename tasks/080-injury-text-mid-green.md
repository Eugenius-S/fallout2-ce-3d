---
id: 080
title: Set the inactive injury label midpoint green
milestone: M1
status: doing
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: [074]
reviewed: no
---

## Goal

Make inactive character injury labels use the midpoint between the original dark green and the current brighter green.

## Context

- The original label color is RGB `(12, 35, 5)`; the current SharpText target is `(0, 123, 0)`.
- The requested arithmetic midpoint is `(6, 79, 3)`, hex `#064F03`.
- Keep active and selected labels, all other colors, Bold 700, and font sizes unchanged.

## Acceptance

- Only the modern inactive injury text color role changes to RGB `(6, 79, 3)`.
- Fade behavior matches the new target and existing color-fade coverage remains.
- A source-bound character capture shows the midpoint color at 2310x1298, scale 2.
- Release build, CTest, and required Python suites pass.

## Out of scope

- Any other UI colors, font weights, font sizes, layout, or game settings.

## Log
