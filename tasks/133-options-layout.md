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

### 2026-11-01 gpt follow-up
Done: Removed the outer frame pixel, unified option row spacing, moved each
section divider below its heading and aligned checkboxes with slider ends.
Moved the scrollbar farther right where space permits and ended its track at
the bottom of the 3D frame on tall screens.
Check: Release build and CTest 18/18; isolated save/cancel probes exercised
scrollbar dragging, the 3D slider and legacy controls. Russian snapshots cover
16:9 and 4:3. The final build's entrance scene matched current origin/main
(e5063d0a) pixel for pixel. Updated outcome images are in runs/133.

### 2026-11-01 gpt control-label and alignment follow-up
Done: Kept legacy preference value text in the original pixel font so the
sharp overlay no longer shifts it behind the knobs. Right-aligned 3D values
in one column and centred checkboxes at the slider endpoint.
Check: CTest 18/18, including a scoped text-capture test. All five primary
knob regions and all six secondary controls match current main's original
pixel text exactly after accounting for the intentional column shift. Isolated
sharp-text save/cancel probes passed; Russian and English snapshots cover 16:9
and Russian 4:3. The entrance scene matches origin/main e5063d0a pixel for
pixel. Earlier screenshot variants remain in the Git history and build folder.

### 2026-11-01 gpt visible-ink alignment follow-up
Done: Right-aligned the actual visible ink of option values in both the pixel
and sharp font, and aligned the rightmost checkbox pixels with slider ends.
The sharp-text alignment anchor follows text through buffer copies.
Check: Clean Release build; CTest 18/18, including actual sharp glyph edges at
output scales 1, 2 and 3 and scoped alignment through buffer copies. On the
1920x1080 Russian captures all 13 visible values ended at physical x=1489 and
all 13 controls at x=1419, both with sharp text on and off. Checked Russian
4:3 and English captures. Sharp-text save and cancel probes passed. The final
entrance scene matched main e5063d0a, whose game and regression sources are
unchanged at current origin/main 59ef4a2a. Earlier images remain in Git
history and build/task133-before-ink-alignment.

### 2026-11-01 gpt settings heading follow-up
Done: Removed both decorative vents from the top of the settings page and
aligned the title with the left settings column, following the owner's second
reference. The earlier screenshot remains in Git history and build.
Check: Release build and CTest 18/18; isolated Russian and English 16:9 and
Russian 4:3 screenshots. The entrance scene matched the main baseline pixel
for pixel; game and regression sources match current origin/main 59ef4a2a.
