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

Only small green NPC conversation replies and player answer text are drawn sharp
at the window's real resolution in a modern font. Every menu, heading, title,
modal header, Pip-Boy, inventory, character, barter, message-log and floating
text font stays pixel-rendered.

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
- Owner correction, 2026-10-23: this narrow conversation-only scope supersedes
  the broader original goal and acceptance list below. A run is eligible only
  when a semantic conversation capture scope is active, its font is 101, and
  its runtime palette index equals `COLOR_GREEN`. Generic color or font
  matching outside the tagged conversation callsites is not sufficient.
- Eligible conversation text uses IBM Plex Mono SemiBold (SIL Open Font License, <https://github.com/google/fonts/tree/main/ofl/ibmplexmono>). Other embedded font weights are unused by this feature. Font sample sheets and the script that makes them: `runs/024-sharp-text-design/`.

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
7. **Fit.** Only tagged font-101 conversation runs are eligible. They use IBM Plex Mono SemiBold.
   Existing generic font rendering code remains available, but other font ids and
   weights are outside this feature.
8. **Font file.** Embedded in the executable by task 026.
9. **Code.** Capture, records, copies, validity and erase:
   `ce/src/scene/text_layer.{h,cc}`, pure CPU code with a unit test suite in
   `ce/tests/text_layer/` (fake buffers: draw, copy, overwrite, scroll,
   overlap). Atlas and GL pass: `ce/src/scene_renderer/text_draw.{h,cc}`.
   Upstream files get only hooks: `text_font.h`, `draw.cc`, `svga.cc`, a
   cursor accessor in `mouse.cc`.

## Acceptance

- Only tagged font-101 conversation replies and player answer text whose
  runtime color is `COLOR_GREEN` are restored and redrawn. Yellow, blue, red,
  title, menu, and other UI text remains pixel-rendered, including all 3D
  options labels and headings.
- English and Russian NPC conversations are captured at x2 and x3 with sharp
  text off and on. Menus are pixel-identical with the setting off and on.
- Sharp dialog text respects window and cursor occlusion, survives scrolling,
  and follows palette fades. Unrelated text is never erased or redrawn.
- The setting is labeled `Sharp dialog text`; off gives the original pixel
  conversation text and the off regression snapshots remain unchanged.
- Unit tests pass. Record the measured preparation time for a conversation
  frame in the `Log`.

## Out of scope

All menus, menu titles, headings, dialog headers, and non-conversation text remain pixel-rendered.

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

### 2026-10-23 gpt (visual fix)
Done: record validity is now atomic across actual legacy glyph deltas; one overwritten glyph pixel drops the complete run while occlusion over blank mask cells leaves it valid. Blit cloning validates glyph deltas in the visible fragment before carrying its geometry, and a new overlapping font draw retires the previous run. Added regressions for partial overwrite, blank-cell occlusion, stale orphan-copy fragments, and legitimate clipped scrolling.
Next: configure after this source commit, full clean Release rebuild and CTest, then repeat only the English x2 Preferences off/on pair for visual review. The first sharp-on pair was rejected for yellow/orange speckles and is retained only as failed evidence; no Russian or broader captures are taken yet.
Check: the stale-fragment fix passed a full clean Release build (719/719), CTest 14/14 and standalone text capture CTest 2/2. The corrected official English x2 Preferences pair is in `runs/025-sharp-text/first-visual-20261023/retry-8f1b9f59/`; both JSON files embed `8f1b9f59` and EXE SHA-256 `e7e2797684cfb29233175e5d242b0949718babb7a96b18f224c4a8ae74f14282`. Sharp off reports inactive; on reports active/drawn with 26 runs, 252 glyphs, 6842 restored pixels and 3.700 ms preparation. The prior speckle scan region has 0 yellow pixels in both images. Await parent visual review before Russian or wider captures.
### 2026-10-23 gpt (cursor occlusion)
Done: screen-cloned runs now validate against their live owner-window buffer generation, while cursor coverage masks only the overlapping cells. The source-to-screen offset is carried through copies. Added tests for cursor coverage over blank and foreground glyph pixels, stale owner generations, and owner-buffer repaint invalidation. The screen buffer remains authoritative for actual restoration, so the cursor pixel is not erased.
Next: full clean Release build and CTest, then official RU x2 and EN/RU x3 Preferences off/on pairs. Wide screen coverage remains untested; the svga presentation orchestration still needs extraction into our module.
Check: reverted screen-only validity temporarily and a clean `text_capture_tests` run failed on foreground cursor overlap; restored owner validation and focused CTest 2/2 plus standalone capture CTest 2/2 passed. Parent independently reviewed this bounded fix. No runtime captures yet for this change.

### 2026-10-23 gpt (localized Preferences runtime pairs)
Done: captured Preferences Top off/on pairs in English and Russian at x3, and Russian at x2, using the official background snapshot helper. Top displays original localized menu strings, so the Russian pairs verify Cyrillic output. Earlier Bottom captures show the hardcoded-English 3D options panel and are English-only evidence.
Next: parent visual review of these localized pairs. The wider task remains in progress; full-screen coverage and extraction of the remaining `svga.cc` presentation orchestration are still pending.
Check: all six captures use the same clean Release executable (source `1037433a`, SHA-256 `cdddd1755b7741f67caa65542cd3ad54f1de9fac2dbc41daa4560de3b673bed6`). Off snapshots report inactive; on snapshots report active/drawn. Evidence base: `runs/025-sharp-text/cursor-runtime-20261023/`. RU x2 `ru-x2-top-off/scr00011.png` and `ru-x2-top-on/scr00012.png` (on: 71 runs, 522 glyphs, 20,912 restored pixels, 7.426 ms). EN x3 `en-x3-top-off/scr00013.png` and `en-x3-top-on/scr00014.png` (on: 76 runs, 468 glyphs, 19,878 pixels, 7.495 ms). RU x3 `ru-x3-top-off/scr00015.png` and `ru-x3-top-on/scr00016.png` (on: 77 runs, 545 glyphs, 20,910 pixels, 11.730 ms). No shared tester process remained after capture.

### 2026-10-23 gpt (known image occlusion)
Done: Preferences option labels are real font calls in `preferences.cc`; the primary switch bitmap is drawn over some glyph pixels. Known tracked image blits now remove only their explicit opaque pixels from older text records, preserving the rest of each run and its original width. Unmatched stale pixels outside known masks still invalidate a run. The `svga.cc` sharp presentation branches now call helpers in `scene_renderer/text_present.cc`.
Next: root review of the focused occlusion algorithm, then a clean build and a revised Russian x3 Top Preferences pair. Do not use runtime evidence from the previous executable for this source change.
Check: official `tools/configure-build.ps1 Release` incremental build passed; focused main CTest `text_layer_tests` and `text_capture_tests` passed (2/2). Tests cover opaque blocks, transparent sprite masks including same-color pixels, source images with no text, stale uncovered glyph rejection and existing text copies. No runtime capture was taken for this change. The full clean rebuild remains pending root review.

### 2026-10-23 gpt (bounded blit tracking)
Done: blit overlap retirement now visits only existing destination text coverage instead of allocating a set for every source pixel. Transparent copies read source pixels only after validating the compact snapshot bounds. Same-buffer clones are still prepared before destination records are clipped.
Next: root review this bounded change; only then run the clean build and revised Russian x3 Top pair. No runtime evidence uses this source yet.
Check: focused `text_layer_tests` and `text_capture_tests` rebuilt with the configured MSVC environment and passed (2/2). Added tests for compact snapshots larger source rectangles (including missing glyph deltas) and the no-text fast path. `git diff --check` passes. Full build and runtime remain pending review.

### 2026-10-23 gpt (font-run overlap)
Done: a new font capture now retires an older run only when their actual changed glyph pixels overlap; overlapping advance/line-height bounds alone preserve both runs. Whole-run atomic validity remains in place, so unknown stale glyph pixels still invalidate the run.
Next: after configure and a full test run, repeat the Russian x3 Top pair and English x2 Bottom ghost check. This source change has no runtime evidence yet; Preferences labels that remain pixel-rendered still need diagnosis.
Check: focused Release `text_layer_tests` and `text_capture_tests` pass (2/2), with fixtures for overlapping bounds/disjoint ink and actual glyph overlap; the existing partial-overwrite atomic-validity regression still passes. `git diff --check` passes. Parent's off regression at `runs/regression/20261023-150351-765-35772/summary.json` reports success for all 16 spots with `update: false`; it used the prior production source `568c5977` and is not evidence for this change.

### 2026-10-23 gpt (font-run overlap verification)
Done: source `2bb6cdd8` passed the configured Release build and full CTest (14/14); Russian Top and English Bottom Preferences off/on pairs were captured with the official background helper. Sharp text reports active/drawn in both on snapshots.
Next: correct the Russian capture at explicit scale 3, then parent visual review. The on/off evidence covers only Preferences; full Pip-Boy, inventory, character screen, dialogue, barter and floating text still need acceptance.
Check: all four snapshots embed `2bb6cdd8` and EXE SHA-256 `7a6246f7841048ae578624df93bc193f83b96f363c9ee9bb2bcaa41937b48ce7`. Evidence base: `runs/025-sharp-text/capture-overlap-2bb6cdd8/`. The folder names overstated scale: RU Top `scr00022.png` is x2 at 1920x1080 physical / 960x540 logical, with 84 runs, 707 glyphs, 43,678 restored pixels and 10.772 ms. EN Bottom `scr00024.png` is x2 at 1280x960 physical, with 26 runs, 252 glyphs, 6,842 restored pixels and 4.720 ms. Off snapshots are inactive; no test process remained after capture. Parent's prior 16-off regression passed all spots with no updates, but used source `568c5977`, before this change.

### 2026-10-23 gpt (text capture trace)
Done: added an opt-in `F2CE_TEXT_TRACE=1` diagnostic for the exact CP1251 bytes of the centered Preferences value label. It reports capture, ink retirement, known blit intersections, copy cloning/skips, source-range misses and atomic validation drops with pixel coordinates and owner generation; it does not change drawing behavior.
Next: parent review of the confirmed transparent-blit source-range miss before any behavior change. Full UI acceptance remains untested.
Check: configured Release build and full CTest passed (14/14). One official background Russian Top Preferences snapshot at 1920x1440 physical, 640x480 logical, scale 3 uses source `dd22715b`, EXE SHA-256 `0bc1f2f34c7cd226eaa676f318a8901dd89ba8970f8eed7e0b6fc872d1c6a015`, and reports active/drawn with 87 runs, 695 glyphs, 43,676 restored pixels and 15.303 ms. Trace evidence is `runs/025-sharp-text/text-trace-dd22715b/ru-top-x3-on/process.stderr.log`. The two centered labels at (68,67) and (68,145) were captured, then touched by transparent primary-switch blits whose source allocation had pitch 640 while the blit pitch was 46. The adapter skipped masking and later rejected each run at x=99 (expected 62, actual 207). No behavior change has been made.

### 2026-10-23 gpt (bounded transparent blit fallback)
Done: when an untracked-pitch source view overlaps actual tracked destination coverage, the adapter now snapshots only the exact blit footprint if it fits inside the known allocation. It uses that pre-blit snapshot to mask transparent destination coverage after the original copy. Unknown or unprovable source ranges keep the conservative path.
Next: parent review of focused tests before any runtime. No visual evidence uses source `10babc3d`.
Check: focused Release `text_capture_tests` passed. The new fixture covers an interior source view with registered pitch 640 and blit pitch 46; same-color and changed opaque pixels are masked, zero pixels remain transparent, and the snapshot is taken before the copy proc. A second fixture extends the source footprint past its registered range and confirms conservative tracking while the pixel copy still runs. Full CTest and runtime are pending review.

### 2026-10-23 gpt (clean build include fix)
Done: added the missing `<cmath>` include required by the embedded-font test's `std::ceil` call, found by the full clean Release build.
Next: configure from this commit, rerun the full clean Release build and CTest, then capture the revised RU x3 Top pair and EN x2 Bottom ghost check if the build passes.
Check: configure-after-`568c5977` passed; the first clean build compiled the application but stopped at `embedded_fonts_test.cc` because `<cmath>` was missing. No runtime capture was run from that build.

### 2026-10-23 gpt (blit fix verification)
Done: source commit `568c5977` passed the complete clean Release build after the isolated `<cmath>` test fix in `36d702fd`. The revised Russian x3 Top pair and an English x2 Bottom ghost check use the official background snapshot helper.
Next: parent visual review and the separate 16-off regression run. Full Pip-Boy, inventory, character screen, dialogue, barter and floating-text acceptance remain untested.
Check: full CTest passed 14/14. Evidence base `runs/025-sharp-text/blit-fix-568c5977/runtime/`; all four JSON files embed `568c5977` and EXE SHA-256 `c2260362335f10648361d1c87dc4ed8223debba24609e465e38d512c5cf05de6`. RU x3 Top on: `scr00018.png`, 82 runs, 617 glyphs, 33,932 restored pixels, 13.489 ms. EN x2 Bottom on: `scr00020.png`, 29 runs, 264 glyphs, 4,921 restored pixels, 6.766 ms. Off snapshots report inactive; on snapshots report active/drawn. No game or tester process remained after capture.

### 2026-10-23 gpt (conversation-only scope)
Done: narrowed capture eligibility to four semantic NPC conversation/player-answer callsites. A run is eligible only with the active Conversation tag, font 101, and runtime COLOR_GREEN; raw capture remains available for diagnostics, while the presentation renderer selects and restores eligible runs only. The setting is labeled Sharp dialog text. Menus and other UI remain pixel-rendered.
Next: configure after this commit, run the full clean Release build and CTest, then verify menu pixels are identical with the setting off/on before designing an actual NPC conversation snapshot fixture.
Check: standalone Release text-layer CTest passed 2/2; git diff --check passed. Parent reviewed and accepted the focused scope/tag diff. Full build and runtime evidence for this scope are pending.
