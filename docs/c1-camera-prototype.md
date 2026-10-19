# C1 prototype: perspective camera

Date: 2026-10-17. Base: fallout2-ce `fb8ba51e` + the `sprite-lighting-c1`
branch (commit `e14218e` in `ce/`). The specification and plan are in
`E:\Games\Fallout 2\docs\superpowers\`.

## What was done

The world is drawn by the vanilla engine into an 8-bit buffer as before; with
the perspective on, every draw (floor, roof, sprite) is projected anew: the
vanilla screen coordinates are treated as the coordinates of a "virtual
ground", and height (roofs, sprite tops) is in vanilla pixels. The camera is
tilted by `pitch` degrees and sits `distance` vanilla pixels from the view
center. The azimuth is vanilla, so sprites show exactly the sides they were
drawn with (clause 46 of the specification).

- `src/lighting/lighting.*` — projection, inverse projection (the mouse),
  render margins; `src/lighting/lighting_quad_blit.*` — a
  perspective-correct quad blitter of the 8-bit palette (index-0
  transparency, light darkening as in `_dark_trans_buf_to_buf`,
  translucency through blend tables).
- The floor keeps the vanilla per-pixel light map (`_intensity_map` is
  filled by the same code, extracted into `tileFillIntensityMap`).
- The mouse: clicks and tile highlight go through the inverse projection
  (`game_mouse.cc`), i.e. click-walking picks the tile that is visually
  under the cursor.
- The **Alt+P** hotkey switches original/perspective on a live scene;
  `[lighting] camera=0|1` in `fallout2.cfg` sets the start mode.
- Scrolling under perspective redraws the whole frame (a linear buffer copy
  with a tilted camera would smear).

## `[lighting]` keys in `fallout2.cfg`

| Key | Default | Meaning |
|---|---|---|
| `camera` | 0 | 0 = vanilla, 1 = perspective |
| `pitch` | 62 | tilt from vertical, degrees (90 = top view) |
| `distance` | 700 | camera distance in vanilla pixels, lower = stronger |
| `roof_height` | 96 | height of the roof plane |

## Evidence (runs/)

- `c1-original.png` — the same save, camera 0 (control, unchanged).
- `c1-persp-weak.png` / a restart with the defaults (62°, 700) — variant 1.
- `c1-persp-strong.png` (55°, 450) — variant 2, stronger.
- `c1-v3-margins.png`, `c1-v3-walk*.png` — iterations of the render margins
  fix.

Comparison: the scene center (the character, the temple entrance) is covered
the same; near objects are larger than far ones; the interface stayed flat;
lighting/palette did not change. Switching ON/OFF does not change the game
rules (a visual layer).

## Known limits of the prototype (not defects of acceptance C)

1. **Void beyond the map edge.** The tilted view shows an area outside the
   map's hex grid: at the ARTEMPLE start there is blackness top-right, and
   its size grows with the perspective strength. There are no tiles there —
   nothing to draw. Solution options (stage C, by agreement): softer
   parameters, a "curtain"/vignette at the edges, drawing the map edges
   (map edge data).
2. **Cursor art in vanilla coordinates.** The red cursor oval and the
   object highlight/outline are drawn in vanilla screen coordinates while
   the picked tile is computed by the inverse projection — at the screen
   edges the oval is slightly off the highlighted tile. Treatment (stage C):
   draw the cursor objects through the same perspective path.
3. **Glass walls/corpse "eggs"** are drawn simplified under perspective
   (without the vanilla special cases).
4. Scroll redraw is heavier than vanilla (the whole frame); FPS measurements
   are stage D, not claimed now.

## How to check by hand

Run: `game\fallout2-ce.exe` (save slot 1 — `--dev-load-game=1`).
Alt+P switches the camera in place; the `[lighting]` section of
`fallout2.cfg` tunes the strength. A click on the ground is a step;
doors/combat/inventory are as in the original.
