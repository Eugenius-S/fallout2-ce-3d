---
id: 085
title: Center the options sliders on their labels' x-height
milestone: M1
status: doing
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Align each 3D options slider's track and knob center vertically with the
x-height center of its label, on the same text row as its numeric value.

## Context

- The owner's screenshot shows tracks below their labels and numeric values:
  `runs/inbox/2026-10-25-slider-track-too-low.jpg`.
- The labels and values start at row + 4, but the track uses a separate fixed
  row + 22 offset. Drawing and dragging already share `sliderTrackY`.

## Acceptance

- Slider tracks and knob centers align with the body of the label text in
  Russian and English options captures under `runs/085-slider-text-alignment/`.
- Use the current font's lowercase ink bounds, not a guessed line-box center.
- Dragging and click hit testing use the same corrected track position.
- Values, text, row spacing and game settings behavior remain unchanged.

## Out of scope

Font changes, other menu redesign and the deferred dialogue hanging indent.

## Log
