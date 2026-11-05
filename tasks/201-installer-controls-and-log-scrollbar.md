---
id: 201
title: Polish installer controls and log scrolling
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: high
tier: 1
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

Finish the installer details identified by the owner after PR #290.

## Context

- Follow-up to installer tasks 199 and 200, both merged. The owner requests
  these corrections now; no open task covers installer controls or log
  scrolling.
- Keep all other installer elements and behavior from task 200.

## Acceptance

- Add left padding inside the install path field.
- Shift the selected language text slightly right to visually center it with
  the dropdown arrow; preserve dropdown selection and saved language.
- Replace standard checkboxes with rounded controls matching the field style.
  All options still toggle correctly, including disabled Restoration Project.
- The Restoration Project label is dark gray, has no light embossed shadow,
  and remains visibly disabled with its checkbox.
- Remove white lines around the progress bar.
- Remove the black inner frame from the log area.
- Replace the log's standard scrollbar with a narrow, rounded yellow thumb,
  without arrow buttons. Hide the thumb when the content fits; show it only
  when the log overflows. Verify actual mouse-wheel scrolling through real
  multiline install/update output.
- Capture current-build English and Russian screenshots in
  `runs/201-installer-controls-and-log-scrollbar/`.
- Build and relevant tests pass; path entry, language dropdown, checkbox
  states, progress, and log scrolling remain functional.

## Out of scope

Other installer layout, install/update/rollback behavior, and game UI.

## Log

### 2026-11-05 gpt

Done: Added left padding to the path edit, shifted the language label for the
dropdown arrow, replaced the three stock checkboxes with rounded owner-drawn
controls, kept Restoration Project visibly disabled and dark, drew the
progress bar without theme stripes, removed the log edit border, and replaced
the native log scrollbar with an overflow-only rounded yellow thumb. The log
supports wheel scrolling and thumb dragging. Evidence is in
`runs/201-installer-controls-and-log-scrollbar/`.

Check: `tools/configure-build.ps1 Release` passed with MSVC 14.40 and Windows
SDK 10.0.22621.0. `ctest --test-dir build --output-on-failure` passed 20/20.
The current Release launcher SHA-256 is
`98E46BC2847885E11AA8D0DF83815BBB6B8622AF093155C3C50A903EE9061E21`.
Fresh English and Russian screenshots are 816 x 759. The path text round-trip
passed; the language popup selected Russian and English and persisted both;
the auto-update checkbox persisted its change; the Restoration Project
checkbox remained disabled after a click. Real multiline output from the
isolated scratch worker overflowed the log, and a physical wheel-up event
moved its first visible line from 3 to 0. The scrollbar was hidden with empty
log content and visible while overflowing. No game files or owner cache were
used.

Next: Review CI results on this commit, then merge the PR when green.
