---
id: 025
title: Sharp text layer
milestone: M1
status: doing
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: [002, 017, 024, 026]
reviewed: no
---

## Goal

Interface text is drawn sharp at the window's real resolution in a modern font,
in the same places and colors as the game's pixel text: Pip-Boy, dialogs,
inventory, character screen, barter, options, the message log, floating text
over critters.

## Context

- The game draws text once into window buffers (8-bit), not every frame:
  every call goes through `fontDrawText` (`ce/src/text_font.h`, 264 call
  sites; the manager's proc is set at `text_font.cc:292`). AAF fonts (ids
  100+) blend 10 brightness levels with the pixel underneath; FON fonts (0-9)
  are 1-bit. Flags: shadowed, underlined, monospaced (`color.h`).
- Text often lands in an off-screen image first (button images) and is copied
  into a window later by `blitBufferToBuffer` / `blitBufferToBufferTrans`
  (`ce/src/draw.cc`). Windows reach the screen surface through the window
  manager; the mouse cursor is drawn into the screen surface too (`mouse.cc`).
- Russian text is CP1251; the Russian fonts come from `mods/rpu_russian.dat`.
- The font, chosen by the owner: IBM Plex Mono SemiBold, titles IBM Plex Mono
  Bold (SIL Open Font License, <https://github.com/google/fonts/tree/main/ofl/ibmplexmono>).
  Font sample sheets and the script that makes them:
  `runs/024-sharp-text-design/`.

## Design

1. **Capture.** `fontDrawText` calls a recorder around the real proc. The
   recorder copies the box the call can write (string width or `length`,
   whichever is smaller, plus one pixel for the shadow; line height plus two),
   runs the proc, and keeps a record: target pointer and pitch, string, font
   id, color and flags, the old-font x position of every character, and every
   changed pixel as offset, value before, value after.
2. **Owner.** A record belongs to the window whose buffer holds its pointer,
   or to the screen surface; otherwise it is an orphan in an off-screen image.
3. **Copies.** The two blits are wrapped: a record inside the source rectangle
   is cloned to the destination after the blit, its before and after values
   read from the destination around the blit. Orphan memory is read only
   inside such a blit, when the game itself reads it; never at present time.
4. **Validity.** At each present a window or screen record lives while all its
   after-values still hold in its buffer; otherwise it is dropped. A new
   record drops older records it overlaps in the same buffer. Records of
   destroyed windows are dropped; the count is capped (oldest first).
5. **Erase.** After `renderPresent` converts the 8-bit frame to RGBA, every
   pixel of a live record whose screen value still equals its after-value is
   replaced with the current palette color of its before-value. The game's
   own 8-bit buffers are never changed.
6. **Draw.** A GL pass after the frame and the 3D scene overlay, at window
   resolution: glyphs from a TTF atlas (the font and stb_truetype come from
   task 026, `embedded_fonts.h`; the atlas is rebuilt when the pixel size
   changes), placed through the same letterbox
   mapping as the frame. Color is the current palette color of the record's
   color index, so fades work. Shadow is one game pixel down-right in black;
   underline at the old underline row. The pass is masked at game resolution:
   only where the record's window is the visible top window (the rule of
   `windowShownMask`) and the mouse cursor is not opaque.
7. **Fit.** Each game font id maps to the chosen family and a weight (titles,
   fonts 103-104, bold). Pixel size: the largest at which the font's cap top
   to descender fits the game's line height times the scale. A string wider
   than its original width is condensed horizontally to it; a monospaced
   string is drawn character by character in its original cells.
8. **Font file.** Embedded in the executable by task 026.
9. **Code.** Capture, records, copies, validity and erase:
   `ce/src/scene/text_layer.{h,cc}`, pure CPU code with a unit test suite in
   `ce/tests/text_layer/` (fake buffers: draw, copy, overwrite, scroll,
   overlap). Atlas and GL pass: `ce/src/scene_renderer/text_draw.{h,cc}`.
   Upstream files get only hooks: `text_font.h`, `draw.cc`, `svga.cc`, a
   cursor accessor in `mouse.cc`.

## Acceptance

- At x2 and x3 scale, Russian and English: Pip-Boy, a dialog with scrolling
  replies, inventory, character screen, barter, options, the message log,
  floating text over critters and button labels after hover and press are
  sharp, in their places and colors.
- No pixel text shows through under sharp text; no sharp text shows over a
  window that covers it or over the mouse cursor.
- A fade to black fades the text with it.
- Text the layer does not capture stays the game's own pixel text.
- `[3d] sharp_text` (default on) with a menu label; off gives the game's own
  text, and the regression snapshots are unchanged.
- Unit tests pass; the frame time of the layer with a full Pip-Boy screen is
  in the `Log`.
- Snapshot pairs (before / after) of every screen above in the PR.

## Out of scope

The main menu's scaled title text (`interfaceFontDrawTextScaled2D`, drawn
outside `fontDrawText`): it stays pixel text unless it is trivial to add.

## Log

### 2026-10-22 gpt
Claim: starting the bounded CPU capture and copy layer in `ce/src/scene/text_layer.{h,cc}`.
