---
id: 197
title: A trial AI HD remaster of one character's standing and walking sprites
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: low
tier: 4
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

A trial, nothing goes into the game: one character's sprites redrawn at x2
and x4 by an image model, to see whether it keeps the original's look and
holds steady from frame to frame.

## Context

- The owner, 2026-11-04, with a news shot about the "FFRS HD Sprite
  Remaster" mod (849 characters and 20,500 frames redrawn with AI at x2 and
  x4; some players felt it lost the original's soul):
  `runs/inbox/2026-11-04-hd-sprite-remaster-reference.png`. Try the same
  on one character, not urgent.
- Stands alone: no open task is an art pipeline experiment.
- One character, for example the player's default look.
- The senior model writes the generation prompt from the original frames:
  what to keep (the palette, the silhouette, the proportions, the light
  direction, the pixel-art feel) and what to sharpen.
- The generation runs on Luna light (`gpt-6-luna`, low effort) with that
  prompt: the standing frame in all 6 directions, and every frame of the
  walk or run cycle in one direction.

## Acceptance

In `runs/197-hd-sprite-trial/`, never in game data:

- An original vs generated sheet at x1, x2 and x4.
- The prompts used.
- A note on frame-to-frame consistency (the animation must not flicker),
  on alignment with the original frame offsets, and on cost and time per
  frame.

## Out of scope

Putting any of it into the game; other characters.

## Log

### 2026-11-04 gpt: default jumpsuit sprite remaster trial

Done: used the default male jumpsuit (`hmjmps`) from `critter.dat`: six stand directions and all eight unarmed walk frames in direction 0. The source sheet retains original FRM direction shifts and per-frame offsets. Prompts and source layout are in `runs/197-hd-sprite-trial/prompts.md` and `frame-layout.json`.

- Original vs generated sheets: `comparison-x1.jpg`, `comparison-x2.jpg`, `comparison-x4.jpg`; transparent AI outputs: `generated-x2.png` and `generated-x4.png`.
- The generator kept the character and the 14-frame layout, but the redraw loses the original pixel-art feel, especially at x4. Treat both outputs as rejected experiments, not game assets.
- At x1, generated walking-frame bounds range from 1.038x to 1.227x source width and 0.916x to 1.063x source height. Their bottom anchors shift upward by 0 to 7 px; this suggests possible jitter, not a verified playback flicker. Measurements are in `metrics.json`.
- The two image requests took about 50 s and 48 s. Dividing by 14 occupied frames gives 3.6 s and 3.4 s per frame as a rough average; generation was per sheet. The tool reported neither backend model variant nor billing, so Luna/low-effort selection and monetary cost cannot be verified.
- No game data was changed.

Check: `py -3 tools/check_public_content.py origin/main` and `py -3 tools/check_runs_size.py origin/main` pass; each image is below 1 MB.
Next: none; this one-character trial is complete.
