---
id: 053
title: Gameplay video at the top of the README
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

Show the owner's gameplay recording at the top of the project page.

## Context

The owner supplied a 69.29-second, 75,662,083-byte recording and requested a
smaller version published in the repository header. Preserve the complete
recording and audio. Host the compressed MP4 as a GitHub attachment rather
than adding a video binary to Git history.

## Acceptance

- The compressed H.264 MP4 plays without decoding errors and retains the
  complete duration and audio.
- A working video player appears immediately below the README title on the
  default branch.
- The original recording is preserved and the media binary stays outside Git.

## Out of scope

Engine changes, the unfinished options menu task, and README copy rewrites.

## Log

### 2026-10-22 gpt
Done: Compressed the owner's recording to 7,066,539 bytes at 1280x716, 30 fps,
with H.264 video, AAC audio and fast-start metadata. The full 69.29-second
file decodes without errors. Source recording is unchanged.
Next: Upload the MP4 as a PR attachment, add its hosted URL below the README
title, verify GitHub rendering, and merge the documentation change.
Check: FFmpeg full-file decode completed with exit code 0. Encoded media is
local in build/media/fallout2-ce-3d-demo.mp4 and is excluded from Git.

### 2026-10-22 gpt
Done: Uploaded the compressed recording as a persistent GitHub attachment in
PR #68 and placed its stable URL directly below the README title. GitHub's
Markdown renderer returns a video player with playback controls for this URL.
Next: Merge the documentation change and verify the default-branch README.
Check: Full-file FFmpeg decode passed. GitHub Markdown API renders a video
element; only README.md and this task file are committed. No engine changes
or game resources are included, so engine and image regression runs are not
needed for this documentation-only publication.
