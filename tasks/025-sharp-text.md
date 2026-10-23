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

### 2026-10-22 gpt (paused by owner)
Done: CPU-only text record draft and focused tests in ce/src/scene/text_layer.{h,cc} and ce/tests/text_layer/, registered in ce/CMakeLists.txt. The parent independently ran text_layer_tests successfully before the pause. No live game hooks or GL text pass are connected. Main checkout was restored to its original main branch after an initial worker cwd mistake; implementation is now in the gpt worktree.
Next: resolve review of transparent-copy coverage (modern glyphs need blank source cells too), add a same-buffer overlapping scroll-copy test, remove unused helpers, add the suite to CI, and rerun tests. Then review the CPU stage before live capture and rendering. Task remains doing and is not accepted.
Check: cmake --build build --target text_layer_tests; ctest --test-dir build -R '^text_layer_tests$' --output-on-failure. Full build, regression and visual acceptance have not run for task 025.

### 2026-10-22 gpt
Done: CPU text records now retain per-pixel glyph clipping, changed-pixel restoration, safe buffer generations, clipped copies, and transparent source coverage. Added fake-buffer tests and the suite to the CI matrix.
Next: parent review this CPU stage; only then start live capture and GL rendering integration. Snapshot helpers must share the live sharp-text pass during that later stage.
Check: cmake --build build; ctest --test-dir build --output-on-failure (13/13 passed). No game process, visual regression, or sharp-text runtime integration was tested.

### 2026-10-22 gpt
Done: added the live capture bridge around fontDrawText, tracked blits, allocator generations, SDL screen storage, window buffers, and OptionsPanel's external content buffer. Captures retain accepted font metrics and mono cell origins; clipped copies keep original logical width. OptionsPanel now submits its visible content as one tracked rectangular blit.
Next: parent review this capture stage. Rendering remains disconnected: GL glyph drawing, erase-at-present, shared snapshot rendering, cursor masking, and window ownership/occlusion provenance still need implementation. Unknown external buffers remain pixel text; no acceptance coverage is claimed. The parent will validate captured options pixels with an official snapshot before the next stage.
Check: Release build via tools/configure-build.ps1; ctest --test-dir build --output-on-failure (14/14 passed). No game process or visual runtime test was run.

### 2026-10-22 gpt (CPU text draw core accepted)
Done: added the CPU glyph rasterizer using embedded IBM Plex fonts, CP1251 decoding, physical-resolution glyph bitmaps, mono cells, shadow and underline handling, and a bounded glyph cache. Visibility masks use compact per-record bounds and plain window/cursor snapshots; hidden or stale owners clear their mask, and standalone capture tests no longer depend on SDL or build-tree headers. Parent reviewed the CPU core.
Next: connect restoration and sharp glyph output to live present and both snapshot paths, then add the setting and opt-in snapshot switch. First runtime comparison remains limited to English and Russian Preferences at x2 with sharp text off/on. No visual erase or GL pass is connected yet.
Check: standalone text-layer Release CTest (2/2), main full CTest (14/14), embedded-font target, and Release `fallout2-ce` build passed. No game process was launched for this core stage.

### 2026-10-22 gpt (paused by owner before limits)
Done: accepted CPU rendering and visibility core is pushed as c837badc. Saved the unfinished live/PNG integration, setting and snapshot-switch draft in a separate checkpoint. Full continuation notes, evidence limits and commands: runs/025-sharp-text/HANDOFF.md.
Next: finish and review the pending integration, fix compilation and settings override ordering, then perform a clean rebuild before any sharp-on evidence. New render logic still needs extraction from upstream svga.cc. No sharp-on game frame has been accepted, no task025 PR is open and no merge is authorized by a passing result yet.
Check: accepted core had standalone capture CTest 2/2, embedded-font tests and worker full CTest 14/14. The interrupted integration has NOT been built or tested. Ninja header dependency tracking is broken locally (zero deps), so an incremental build is insufficient evidence. See HANDOFF.md for the complete Acceptance sequence and runtime hash caveat.

### 2026-10-23 gpt
Done: moved live capture, restore, palette expansion, GL upload and one-time failure shutdown into `ce/src/scene_renderer/text_present.cc`; `svga.cc` retains short calls. Sharp output uses an independent GL texture, both PNG paths reuse the prepared pass, and sidecars report setting, active/drawn state, run/glyph counts, restored pixels and preparation time. `-SharpText` remains opt-in for snapshots.
Next: parent visual review of the source-verified English x2 Preferences pair, then take the Russian x2 off/on pair. Wider task acceptance remains untested.
Check: configure after commit `cd571a2f`, full Release clean rebuild (719/719), CTest 14/14, standalone text capture CTest 2/2. Official background snapshots under `runs/025-sharp-text/first-visual-20261023/source-verified/english-{off,on}` share EXE SHA-256 `da0634f95c6c52e11f943f5e9cabfee41a186fe8f70a70ba557b60b671fe9b38` and embed commit `cd571a2f`; on reports active/drawn, 86 runs, 798 glyphs, 6842 restored pixels, 7.929 ms prepare time. An earlier off snapshot with stale embedded commit `59efdd34` remains preserved outside `source-verified` and is not evidence.
