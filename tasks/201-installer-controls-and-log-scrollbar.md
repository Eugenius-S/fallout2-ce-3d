---
id: 201
title: Polish installer controls and log scrolling
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: high
tier: 1
claude-only: no
user-check: yes
depends-on: []
reviewed: no
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
