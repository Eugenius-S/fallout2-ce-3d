# 218 — the region mark takes the owner's real drag (the -2 mouse frames)

Status: in_progress
Owner report 2026-11-07 (their Ctrl+F12 shots 20261107-1747*: both JSONs
`"regions": null`, first attempt «рамка отменена», second «рамка сохранена»
with an empty marked PNG): the mark mode opened (the Russian hint showed) but
no drag ever produced a rectangle.

## Root cause

`inputGetInput()` answers **-2, not -1**, on every frame a mouse button goes
down, repeats or comes up - it checks the same `gMouseEvent & 0x33` flags
itself (input.cc). The mark only watched for -1 (`owner_shot::handleKey`),
so `handleMarkMouse()` never ran on the exact frames a drag consists of. A
real drag could not register, ever.

The 217 verification missed it because `pumpMark()` pushed its rectangles
straight into `gMarkRects` and called `saveMark()` - the input path was never
exercised.

## Fix

- `owner_shot::handleKey` treats -1 and -2 alike while the mark mode holds
  the input: `handleMarkMouse()` runs, the game still gets nothing.
- A new bench, `--dev-owner-mark-drag=N`: waits for the settled view, enters
  the mark mode, replays a drag through the **SDL queue** (the scripted
  input's fake mouse device, now usable without a script file via
  `scriptedInputSetExternalMouse` - the observer and the device override both
  gate on it), presses Enter, and prints two verdicts: the rectangle
  registered, and the green outline is **in the presented pixels** (a
  readback of the framebuffer strip through
  `scene_renderer::overlayLogicalToWindow` + `readPresentPixels`, baseline
  before the press vs after).
- `--dev-new-game`/`--dev-load-game` are what gets a dev run past the main
  menu; `--dev-map` alone sits in the menu (bench gotcha, confirmed the hard
  way twice).

## Verification

- ctest 20/20 green.
- Demo run, kladwtwn e0, `--dev-owner-mark-drag=5`:
  `mark drag rect registered (1 rectangles)`, `overlay pixels verified on
  screen (green 640, baseline 0, strip 1281)`, and the saved JSON carries the
  dragged `regions` rect with live objects (the auto-invisibility cages).
  Before the fix the same bench printed `FAILED: drag never registered`.

Feeds task 157: the owner's own shots put kladwtwn e0 at 21.6-22.6 ms/frame
(41-46 fps) standing still - the fps work continues there.
