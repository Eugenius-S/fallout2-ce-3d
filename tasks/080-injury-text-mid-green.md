---
id: 080
title: Set the inactive injury label midpoint green
milestone: M1
status: done
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: [074]
reviewed: yes
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

### 2026-10-25 gpt
Done: changed only the inactive injury label target to RGB `(6, 79, 3)` (`#064F03`). Updated the existing intermediate fade expectation to `(3, 41, 1)`; active, selected, other colors, Bold 700, and font sizes are unchanged.
Check: official MSVC Release clean build passed 736/736; CTest passed 15/15; exporter passed 50/50 and regression Python tests 12/12. Character fixture PASS at 2310x1298, scale 2, source `3a3c7e91`, EXE SHA-256 `b268dd121f0046f8b5820d5813e9cdca609809f0f2d493ec32b7543719c15202`. The raw capture has 3946 exact midpoint pixels and preserves 605 active-green pixels. JPEG crop and sidecar are under `runs/080-injury-text-mid-green/accepted/README.md`; raw PNG is local at `<game>\shots\raw-task080\scr00205.png`.
Check: SharpText-off regression passed all 16 spots with 0 changed pixels and no reference updates: `runs/regression/20261025-155225-745-47476`; every capture uses source `3a3c7e91` and the same EXE SHA-256.
Check: the snapshot wrapper's post-capture copy looked for the PNG beside the JSON in `shots/dev`; the exact sidecar points to `shots/game/scr00205.png`, which was used for the accepted crop. The game capture itself passed.
Next: root review and merge.
