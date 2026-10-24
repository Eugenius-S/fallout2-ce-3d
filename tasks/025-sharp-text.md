---
id: 025
title: Sharp green dialogue text
milestone: M1
status: done
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: [002, 017, 024, 026]
reviewed: yes
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
- Decoder boundary, 2026-10-24: captured bytes are decoded as CP1251, so sharp
  text supports only the canonical `english` and `russian` language settings,
  compared case-insensitively. French, German, Italian, Spanish, empty and
  unknown language values must keep original pixel text and disable the sharp
  text setting with one diagnostic before any restoration or draw.
- Eligible conversation text uses IBM Plex Mono SemiBold (SIL Open Font License, <https://github.com/google/fonts/tree/main/ofl/ibmplexmono>). Other embedded font weights are unused by this feature. Font sample sheets and the script that makes them: `runs/024-sharp-text-design/`.

## Design

1. **Capture.** `fontDrawText` calls a recorder around the real proc. The
   recorder copies the box the call can write (string width or `length`,
   whichever is smaller, plus one pixel for the shadow; line height plus two),
   runs the proc, and keeps a record: target pointer and pitch, string, font
   id, color and flags, the old-font x position of every character, and every
   changed pixel as offset, value before, value after.
2. **Owner.** A record is associated with the window buffer and generation
   that owns its pixels, or with the screen surface; otherwise it is an orphan
   in an off-screen image. The owner's current visibility and occlusion mask
   determine which parts can be restored and drawn.
3. **Copies.** The two blits are wrapped: only a record intersecting the
   source rectangle is cloned, clipped to the copied source and destination
   regions. Its before and after values are read from the destination around
   the blit. Orphan memory is read only inside such a blit, within a known
   allocation range the game itself reads; unknown ranges are not sampled.
4. **Validity.** At each present a window or screen record lives while all its
   changed ink pixels still hold their after-values in the owning buffer;
   otherwise the complete run is dropped. A new record retires an older run
   only where changed ink pixels actually overlap in the same buffer; overlap
   of their bounding boxes alone is not enough. Records of destroyed or
   repainted owner generations are dropped; the count is capped (oldest first).
5. **Erase.** After `renderPresent` converts the 8-bit frame to RGBA, every
   pixel of a live record whose screen value still equals its after-value is
   replaced with the current palette color of its before-value. The game's
   own 8-bit buffers are never changed.
6. **Draw.** CPU-rasterized TTF glyph bitmaps are composed into a physical-size
   RGBA overlay, then uploaded through the existing GL overlay texture after
   the restored frame and 3D scene overlay. The bounded glyph bitmap cache is
   keyed by glyph, physical size and weight; there is no GPU TTF atlas. Glyphs
   use the same letterbox mapping as the frame. Their current palette color is
   resolved on each prepared frame, so palette fades change the ink while
   leaving glyph coverage and placement unchanged. Shadow is one game pixel
   down-right in black; underline at the old underline row. The pass is masked
   at game resolution: only where the record's owner window is the visible top
   window (the rule of `windowShownMask`) and the mouse cursor is not opaque.
   Physical pixels outside the frame are clipped before composition.
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
- Sharp text accepts only English and Russian. Other, empty and unknown
  language values are rejected before text restoration or drawing; the setting
  turns off with one diagnostic and original pixel text remains.
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

### 2026-10-24 gpt
Done: verified English x2 and Russian x3 Preferences Top snapshots with sharp dialog text off/on. Both pairs are pixel-identical; the on snapshots report the setting enabled but inactive, with zero captured or restored runs. The clean Release build after `80d57da5` rebuilt all 175 game objects; the built EXE matches the snapshot SHA-256 `fb5a368a652cbd794c536637808373ef2b972fc57a533f6737e7262cdd8880e7`.
Next: parent review a real NPC conversation fixture design before implementation. Menus, titles, Pip-Boy and all other UI remain outside sharp text scope.
Check: full CTest passed 14/14; decoded RGBA images match exactly for `runs/025-sharp-text/menu-scope-80d57da5/en-x2-top-{off,on}` and `ru-x3-top-{off,on}`. Snapshot metadata reports scale 2 for English and scale 3 for Russian.

### 2026-10-24 gpt (dialog fixture and palette fade)
Done: added an opt-in real Klamath Sulik conversation fixture using the loaded `Kcsulik.int` script and normal talk-range check. Snapshot readiness requires eligible runs from separate reply and answer owner windows. Corrected the renderer design notes to describe CPU-rasterized glyph bitmaps, physical RGBA overlay, actual-ink overlap, clipped copies and owner-generation visibility. Added an embedded-font regression that changes the active palette color from green to black and asserts glyph coverage stays identical.
Next: capture the committed-source English x2 dialog off/on pair for parent visual review. Wider locale and scale coverage waits for that review.
Check: commit `6a4a86ed` was configured and fully rebuilt with the official MSVC setup; full CTest passed 14/14. The focused `embedded_fonts_tests` rebuild and CTest passed 1/1. Official snapshots under `runs/025-sharp-text/npc-dialog-20261024/committed-en-x2-{off,on}-6a4a86ed/` both embed `6a4a86ed`, use the same EXE SHA-256 `3173879bcdae46afd9ee328d0e2be5bbe5cc0921e981f46bd6d9dcd86f61412c`, and report 1920x1080 physical / 960x540 logical at scale 2. Both fixture logs confirm Sulik (`Kcsulik.int`, speaker 26 at tile 23478), hero tile 25075, `talk_result=0`, owner windows 7 and 8, and 8 eligible runs. Sharp-off reports inactive with zero runs/restores; sharp-on reports active/drawn, 9 runs, 293 glyphs, 4,639 restored pixels and 5.159 ms preparation. Menu pairs remain separately verified as pixel-identical.

### 2026-10-24 gpt (dialog locale and scale matrix)
Done: captured the real Sulik dialog off/on at English and Russian x2, plus English and Russian x3. Each uses the same committed executable, map, speaker, hero placement, and normal talk-range check.
Next: parent visual review of the locale/scale matrix. Propose a targeted real-cursor-over-glyph check using the engine mouse setter, then a real reply-page check using the game's KEY_2 dialogue answer and KEY_ARROW_DOWN paging input. Do not add either runtime mode until the exact reply offset/state is confirmed and the parent reviews the plan.
Check: all eight official snapshots embed `6a4a86ed` and EXE SHA-256 `3173879bcdae46afd9ee328d0e2be5bbe5cc0921e981f46bd6d9dcd86f61412c`. English and Russian x2 PNGs are 1920x1080 (logical 960x540); x3 PNGs are 1920x1440 (logical 640x480). Each fixture log confirms `Kcsulik.int`, `talk_result=0`, two owner windows and eligible records. OFF remains inactive with zero runs/restores. ON reports EN x2: 9 runs, 293 glyphs, 4,639 restored pixels, 5.159 ms; RU x2: 11, 379, 5,533, 5.123 ms; RU x3: 11, 379, 5,533, 6.179 ms; EN x3: 9, 293, 4,639, 6.094 ms. Evidence is under `runs/025-sharp-text/npc-dialog-20261024/committed-{en-x2,ru-x2,ru-x3,en-x3}-{off,on}-6a4a86ed/`.

### 2026-10-24 gpt (runtime cursor masking)
Done: inspected an actual Kcsulik reply after one engine `KEY_2`; its `dialogReplyTextOffset` is 0, and its two visible lines contain 61 bytes. No page-down was injected, so this does not verify a paged live reply. Added a gated runtime cursor check that puts the engine cursor over a real sharp reply glyph pixel, verifies that pixel is masked and that other glyph pixels in the same reply run remain visible, then takes a framebuffer snapshot.
Next: parent review of the committed cursor proof, then run the final 16-case sharp-off regression. Keep CPU clipped-scroll test coverage distinct from runtime paging acceptance; no long live NPC reply with nonzero offset has been observed.
Check: after source commit `2af21dea`, official Release configure/build and CTest 14/14 passed. The helper report at `runs/025-sharp-text/cursor-runtime-20261024/2af21dea-en-x2/process.stdout.log` records target alpha 126 to 0 and same-run visible glyph pixels 2601 to 2467. Its JSON binds EXE SHA-256 `42bcc40ea09a6d510500b7d8ce12a45354721aa0bfc30993b6c8dddfcf2fd3ed`, reports sharp text active/drawn, 6 runs, 225 glyphs, 1,835 restored pixels and 2.173 ms preparation at 1920x1080 / x2. PNG and JSON are curated beside the log.

### 2026-10-24 gpt (cursor proof rejected)
Done: parent rejected the previous screenshot proof. Its engine cursor stayed at the original center; the reported target alpha change only tested the mask at requested coordinates and did not prove a cursor blit there. Kept the old artifacts, marked them rejected, and fixed the gated check to hide, move and redraw the engine cursor, verify the opaque indexed framebuffer pixel and cursor rectangle, then compare the saved PNG pixel color.
Next: rerun the official English x2 cursor fixture from commit `1c00e08b` after the shared tester is free. Final regression and task acceptance remain pending.
Check: official Release configure/build after `1c00e08b` passed; full CTest passed 14/14. Runtime proof is pending.

### 2026-10-24 gpt (cursor redraw verified)
Done: parent independently confirmed the corrected cursor proof. The official fixture draws the real engine cursor over a green Kcsulik reply glyph; its indexed source pixel and palette color match the decoded PNG, and the sharp overlay masks the cursor point while other glyph ink remains visible.
Next: parent is running the final 16-case sharp-off regression and CTest. Keep task status `doing` until those checks and final review are reported.
Check: source commit `1c00e08b`, executable SHA-256 `fdd9ed58c09d67c9e101d525b0f3471bfa587943a7574466c32e483dfb41f8f2`; CTest 14/14 passed. Validated evidence is `runs/025-sharp-text/cursor-runtime-20261024/1c00e08b-en-x2-validated/`: 1920x1080 PNG at x2, target physical pixel (610,530) is RGB (84,84,84) in both expected palette and decoded PNG, overlay alpha 126 to 0, and same-run visible ink 2601 to 2408. The older `2af21dea` cursor image is explicitly rejected: its cursor stayed centered and its alpha assertion did not prove a real cursor blit.

### 2026-10-24 gpt (accepted)
Done: completed the conversation-only sharp text scope. English and Russian Klamath NPC reply/answer captures cover x2 and x3 with the setting off and on; the menu off/on evidence is pixel-identical. The corrected real-cursor proof is accepted. The final sharp-off regression passed all 16 spots with zero changed pixels, and the full CTest passed 14/14. The 44/44 exporter and 12/12 regression unit suites also passed.
Next: parent updates the draft PR #77 after the latest CI result, then handles PR readiness and merge.
Check: final regression summary is `runs/regression/20261024-103814-975-9564/summary.json` and uses source `1c00e08b`, EXE SHA-256 `fdd9ed58c09d67c9e101d525b0f3471bfa587943a7574466c32e483dfb41f8f2`. Curated evidence and the exact scope limits are in `runs/025-sharp-text/final-validation-20261024.md`. No live long-reply paging claim is made; the inspected real reply had offset 0, while scroll and palette-fade behavior are covered by focused CPU tests. The rejected `2af21dea` screenshot remains marked as rejected above.

### 2026-10-24 gpt (CP1251 locale boundary)
Done: reopened the task after PR #77 review identified that unconditional CP1251 decoding would render accented French, German, Italian and Spanish dialog bytes as Cyrillic. Added an explicit English/Russian language gate before text restoration, palette expansion and drawing; unsupported values follow the existing one-shot failure policy and retain pixel text. Added focused language-policy tests.
Next: parent reviews the focused diff and decides whether to request an English or unsupported-language runtime smoke. The existing English/Russian matrix remains valid; no matrix rerun is needed for the decoder boundary.
Check: commit `5cde8f97` passed official Release configure/build, targeted `embedded_fonts_tests` 1/1 and full CTest 14/14. `settings.system.language` is a `std::string` defaulting to canonical `ENGLISH` (`"english"`). The unit predicate accepts English/Russian case-insensitively and rejects French, German, Italian, Spanish, empty and unknown values. In `prepareSharpTextForPresent`, the gate precedes restoration, palette expansion and drawing, preserving the indexed pixel buffer for unsupported languages. No runtime smoke was run.

### 2026-10-24 gpt (final validation)
Done: completed and reviewed the bounded English/Russian NPC conversation text scope. Only small green NPC replies and player answers use sharp text; all menus and other UI remain pixel-rendered. Added owner-validated capture through cursor compositing and the conservative language gate.
Next: parent handles PR #77 readiness and merge after CI.
Check: game source `5a8c0183` was built with the official Release configuration; full CTest passed 14/14. The final official English x2 cursor fixture binds to that source and EXE SHA-256 `fa5b4c618a35c4436ff7857f2218105f2994454143477236081b8df88005582e`. At physical cursor pixel (610,530), decoded PNG RGB matches cursor palette `(84,84,84)`; overlay alpha changes `126->0`. At transparent pixel (637,533), inside the same cursor rectangle, sharp text alpha stays `5->5`; remaining same-run ink is `2601->2467`. Evidence is `runs/025-sharp-text/cursor-runtime-20261024/5a8c0183-en-x2-transparent-verified-v2/`. The final sharp-off regression on embedded source `5a8c0183` passed 16/16 with zero changed pixels; summary: `runs/regression/20261024-111711-398-28976/summary.json`. English/Russian off/on NPC captures cover x2/x3, menu off/on pairs are pixel-identical, exporter tests passed 44/44 and regression unit tests 12/12. CPU tests cover clipped scrolling and palette fades. The inspected live reply had offset 0, so runtime paging is not claimed. Rejected earlier cursor proofs remain explicitly identified in this log and handoff.
