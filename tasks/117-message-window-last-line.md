---
id: 117
title: The message window's last line shows whole
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The bottom line of the message window under the interface bar shows in
full, not cut at its foot.

## Context

- The owner, 2026-10-28, live on main `ff5f4ed0`, Russian text:
  `runs/inbox/2026-10-28-chat-bottom-line-cut.png`. The lowest line's
  letters lose a pixel or two at the bottom.
- Check whether the original game cuts it the same way with this font
  (the Russian font's descent against the window's line height) before
  changing anything; if it is ours (the text layer or the 3D build's
  scaling), fix ours.

## Acceptance

- before | after of the message window full of lines, in
  `runs/117-message-window-last-line/`, at 16:9 and 4:3.
- The original view regression stays pixel for pixel unless the cut is
  ours there too; ctest passes.

## Out of scope

The message window's art, font choice, line count and text wrapping.

## Log

### 2026-10-29 gpt
Done: Added a -2 logical-pixel SharpText offset for font-101 HUD message runs. Extended the log fixture to render six Russian lines, ending with the owner's message, and added a capture-metadata assertion. Before/after evidence: `runs/117-message-window-last-line/before-after-16x9.jpg` and `runs/117-message-window-last-line/before-after-4x3.jpg`.
Check: Release build passed; text capture unit test passed; Python suites passed (67 scene-export tests and 12 regression tests). `ctest` passed 17/18; `settings_3d_tests` still fails its existing clean_edges default assertion. Full pixel regression passed 11/16; entrance (191092), map-edge (453853), klacanyn-e0 (246157), klagraz-e0 (42372), and klaratcv-e2 (6299) changed pixels.
Game cache: An exporter invocation without `--cache-root` wrote 11 generated files under `E:\Projects\fallout2-ce-3d\game\mod-cache\scene\kladwtwn\0`. No verified pre-run backup exists; the files were left in place and are not included in this branch.
Next: Reconcile the failing baseline checks and regression references before marking this task done.
