# 017: the owner's layout for the options screen (2026-10-22)

Replaces the look in `tasks/017-options-menu-3d-panel.md`; the model layer
already done stays. Copy this into the task's Context and Acceptance, and the
owner's pictures into `runs/017-options-menu-3d-panel/`:

- `<worktree>\images\54.webp`: the original block, kept as is.
- `...\images\55.png`: the scroll bar style (the original slider groove and its red knob).
- `...\images\56.png`, `...\images\57.png`: the screen frame style (the message window and the dialog screen).
- `...\images\58.png`: the first attempt (a flat green panel over the map): not the wanted look.

## The look

1. The whole game screen behind the menu is metal: a seamless patch of the
   preferences art's metal, tiled to fill the screen at any resolution. The
   map does not show through.
2. At the top, the original preferences block exactly as it is now (knobs,
   sliders, its three buttons), every control working as before.
3. Below it, a screen built like a web 9-slice from a game frame: best the
   big Pip-Boy's screen (the owner's pick), else the message window or the
   dialog screen. Four corners, the edges tiled, the inside tiled. It grows down with the number of 3D rows, so a new setting needs no
   art change. The rows inside in the green Pip-Boy text, as in the first
   attempt.
4. On the right, a vertical scroll bar in the original slider style: the
   groove tiled along the height and the red knob. It scrolls the whole page
   (the original block and the screen below it) when the page is taller than
   the window. Wheel, dragging the knob, clicking the groove and the keys
   (arrows, PgUp, PgDn, Home, End) scroll it.

## Rules

- The pieces are cut at run time from the game's own FRMs (rectangles as
  constants in our code); no game art is committed.
- If a frame proves hard to cut, a plain temporary frame is fine as an
  intermediate commit, not as the end of the task.
- Snapshots in the PR: x2 and x3 scale, Russian and English, scrolled to the
  top and to the bottom.
