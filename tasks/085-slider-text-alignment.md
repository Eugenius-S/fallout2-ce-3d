---
id: 085
title: Center the options sliders on their labels' x-height
milestone: M1
status: done
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
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

### 2026-10-25 gpt

Done: replaced the fixed slider offset with the font-101 lowercase ink center
in `ce/src/scene/options_panel.*`. Pixel text measures an off-screen `x`;
sharp text uses a renderer metrics helper in `scene_renderer/text_draw.*`
with the actual font weight, preview size and output scale. Drawing and mouse
handling share the corrected `sliderTrackY`. Labels, values and spacing are
unchanged. Evidence: `runs/085-slider-text-alignment/`.

Next: no remaining work in 085. The dialogue hanging-indent item stays in the
inbox; this task changes only slider placement and its metrics/tests.

Check: Release build, CTest 15/15, exporter tests 55/55, regression unit tests
12/12. New embedded-font checks compare the center with actual rendered ink
in 40 weight/size/output-scale combinations. Russian and English captures
show the alignment. Pixel and sharp-text mouse/cancel probes pass with matching
before/after config hashes. All 16 world scenes match the temporary current-base
control references from 066 with 0 changed pixels. Old committed references
remain untouched (their drift is task 058). No owner game was restarted.
