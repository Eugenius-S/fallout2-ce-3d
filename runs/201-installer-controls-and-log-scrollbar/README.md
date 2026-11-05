# Installer controls and log scrolling

Captured from the Release launcher built in this worktree on 2026-11-05.
All four JPEGs are fresh `PrintWindow` captures at 816 x 759 and below 1 MB.
`capture.json` records the launcher SHA-256 and language for each frame.

- `installer-en.jpg` and `installer-ru.jpg`: English and Russian layouts,
  with the bold folder heading and normal-weight `master.dat` / `critter.dat`
  hint between the logo and path edit. The edit is populated from a scratch
  game folder.
- `progress-active.jpg`: the custom progress bar while the isolated worker
  was running. It has no themed white stripe.
- `log-scroll.jpg`: real multiline worker output, with the yellow thumb at the
  top after scrolling upward.
- `interaction-check.json`: the edit text round-trip, option clicks, settings
  persistence, language popup selection in both directions, disabled
  Restoration Project checkbox, overflow visibility, and mouse-wheel result.
  A physical `MOUSEEVENTF_WHEEL` event over the log moved the first visible
  line from 3 to 0. The scrollbar stayed visible while the log overflowed.
  Its initial hidden state is recorded in `capture.json`.

The worker used only `build/task201-scratch-game` and an unreachable loopback
release endpoint. It stopped before downloading or changing any game files.
The owner's game and cache were not used.
