---
id: 212
title: The load dialog shows each save's preview picture
milestone: M1
status: open
owner: gpt
suggested: owner
priority: medium
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The owner, 2026-11-07: "there is no save preview when loading." The load
dialog must show the picture of the save slot being picked.

## Context

- The engine captures a preview at save time (a 640x380 area of the
  isometric window, loadsave.cc) and the load dialog draws it back.
- Suspects: saves made while the 3D scene was active capture the isometric
  framebuffer that the 3D presentation does not draw into, so the stored
  preview is black or stale; or the preview drawing path fails at the
  owner's current resolution/scale.
- Needs one headless repro on the demo copy: save with the 3D view active,
  open the load dialog, capture. Then fix whichever half loses the picture
  (capture from the presented frame at save time, or draw it at load).

## Acceptance

- A save made in the 3D view shows its picture in the load dialog.
- A save made in the classic view keeps showing its picture.

## Log

- 2026-11-07: owner report; the capture-at-save suspect recorded; repro
  pending.
- 2026-11-07, later: the owner is right that this was implemented — gpt's
  branch task/134-save-preview-3d-gpt (capture and reload 3D save previews,
  the evidence gap closed) never reached main. The old PR 297 sat draft
  with conflicts. Rebased onto current main (the shared readback helpers
  resolved in favor of the 183 versions, the preview capture kept as a
  thin wrapper), build verified, PR 297 refreshed and marked ready.
