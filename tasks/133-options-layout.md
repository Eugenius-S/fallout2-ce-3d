---
id: 133
title: Refine the settings window layout and frame
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

Apply the owner's settings window layout and edited frame pieces, improve the
3D option labels and grouping, and keep camera rotation off by default.

## Context

The owner supplied reference screenshots and eight edited frame pieces on
2026-11-01. Keep the supplied originals unchanged. Leave extra room beneath
headings, round the cut-out blocks and separate groups with dividers.

## Acceptance

- Use the owner's frame pieces and show the resulting window in Russian.
- Rounded legacy blocks, clear spacing and dividers between 3D groups.
- Review option wording, including the sun path angle, and verify the camera
  rotation default remains off without resetting the player's saved choice.
- Check the window and its controls in an isolated game copy and compare the
  original view with current main. Save outcome screenshots under runs/133.

## Out of scope

Changes to the owner's active game or unrelated rendering.

## Log

### 2026-11-01 gpt
Done: Used the owner's eight edited frame pieces, rounded the legacy cutouts,
shifted the two left legacy columns below the heading on tall screens, reduced
3D heading size, spaced group headings and added dividers. The Russian labels
clarify the sun path angle and game lighting. Camera rotation already defaults
to off; existing saved values are preserved. At 640x480 the original column
position is retained to keep the footer usable.
Check: Release build and CTest 18/18; isolated preferences save/cancel probes
cover the shifted legacy control, 3D slider, scrolling and footer buttons.
Sixteen regression scenes matched current origin/main e5063d0a pixel for pixel
on the pre-divider-color build; the final build's entrance also matched.
Russian outcome captures are in runs/133-options-layout.
