---
id: 205
title: The F12 screenshot actually writes into shots/game
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

# 205: The F12 screenshot actually writes into shots/game

## Goal

The owner pressed F12 and got 0-byte `scr00311.png`-style files. The PNG
screenshot handler (task 052 moved shots to `shots/game/`) probed and created
`shots/game/scrNNNNN.png` with the full path, but then handed
`lodepng::encode` the bare filename: the real PNG went next to the exe (the
process CWD) and the file in `shots/game/` stayed at 0 bytes, which is exactly
what the owner found. The BMP default handler always used
`dev_output::outputFile` for both probe and write and was never broken.

The fix: drop the stub-create entirely and encode straight to
`(directory / fileName)`.

## Context

- Found during the feature audit after the owner's "не работает" run:
  `shots/game/scr00311-313.png` were 0 bytes on disk.
- The stub-create + encode-to-CWD pair also meant every failed encode left a
  fresh 0-byte file behind, so the dir filled with them.

## Acceptance

- [x] A hidden scripted-input run presses F12 in game and a non-empty
  `shots/game/scrNNNNN.png` appears; no new `scrNNNNN.png` appears next to the
  exe.
- [x] Build and tests green on CI.

## Out of scope

- The 3D save preview (task 134, still queued).

## Log

### 2026-11-07 gpt
Done: input.cc encodes to the full path, stub-create removed. Verified in the
owner demo copy with a scripted F12 press. Next: CI, merge, done.
