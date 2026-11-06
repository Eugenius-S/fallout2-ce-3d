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
- Move the folder heading upward, make it bold, and add a persistent normal-weight
  RU/EN hint directly below it explaining that the selected folder needs
  `master.dat` and `critter.dat`.
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
progress bar without theme stripes, matched the read-only log EDIT background
to its panel (including its `WM_CTLCOLORSTATIC` path), and replaced the native
log scrollbar with an overflow-only rounded yellow thumb. The log supports
wheel scrolling and thumb dragging. Evidence is in
`runs/201-installer-controls-and-log-scrollbar/`.

Check: `tools/configure-build.ps1 Release` passed with MSVC 14.40 and Windows
SDK 10.0.22621.0. `ctest --test-dir build --output-on-failure` passed 20/20.
The current Release launcher SHA-256 is
`BB75D95E94A7E5C1F92B98AE78F6E28C209A4B09E252B32FC8542B557B8B7B4C`.
Fresh English and Russian screenshots are 816 x 759. The path text round-trip
passed; the language popup selected Russian and English and persisted both;
the auto-update checkbox persisted its change; the Restoration Project
checkbox remained disabled after a click. Real multiline output from the
isolated scratch worker overflowed the log, and a physical wheel-up event
moved its first visible line from 3 to 0. The scrollbar was hidden with empty
log content and visible while overflowing. No game files or owner cache were
used.

Next: Review CI results on this commit, then merge the PR when green.

### 2026-11-05 gpt

Done: The owner added a visible path hint to the same installer follow-up.
Moved the folder heading up and made it bold, then placed the normal-weight
localized `master.dat` / `critter.dat` instruction between it and the path
field. RU/EN captures and checks are pending.

Next: Verify that the helper line fits in both languages, rebuild and test,
then replace evidence and wait for CI on the final SHA.

### 2026-11-05 gpt

Done: Verified the new bold folder heading and normal-weight localized hint
fit between the logo and path field in both languages. Replaced all current
build evidence. The log remains a single task with no added layout changes
beyond the owner's requested hint.

Check: `tools/configure-build.ps1 Release` passed with MSVC 14.40 and Windows
SDK 10.0.22621.0. `ctest --test-dir build --output-on-failure` passed 20/20.
The current launcher SHA-256 is
`9590116F728A4B93ED72CB28586641577A80BF00241F8B637C7E2CEFD0CE44EE`.
Language popup persistence, both checkbox states, path edit, overflow-only
scrollbar and physical wheel scrolling passed. Fresh RU/EN, progress and log
captures are in `runs/201-installer-controls-and-log-scrollbar/`.

Next: Review CI on the final SHA, then merge the PR when green.

Check: See the Release build, CTest output, current-build images, and
`interaction-check.json` under `runs/201-installer-controls-and-log-scrollbar/`.

### 2026-11-05 gpt

Done: The owner found that the hint sat too close to the path field border.
Moved the heading and hint upward while keeping both below the logo.

Check: Rebuilt Release with MSVC 14.40 and Windows SDK 10.0.22621.0; CTest
passed 20/20. Fresh RU/EN captures show the full gold field outline and a gap
below the hint. The current launcher SHA-256 is
`B2F26B1F984991DA8F371CAE0E952C4BB6F4C690516327D8715BA534B572754D`.

Next: Review CI on the final SHA, then merge the PR when green.
