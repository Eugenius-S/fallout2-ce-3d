# Task 025 handoff (owner paused for limits, 2026-10-22)

## Workspace and execution

- Repository: Eugenius-S/fallout2-ce-3d. Worktree: E:/Projects/fallout2-ce-3d-gpt. Branch: task/025-sharp-text. Never switch the main checkout branch.
- User requires exactly one implementation worker: gpt-6-luna with High reasoning. Parent reviews and accepts sequential bounded stages. Existing worker: /root/luna_025, interrupted at the pause. Reuse it if available; do not fan out.
- Read AGENTS.md and tasks/025-sharp-text.md. Preserve all existing untracked task017 evidence and local build folders. No cleanup is needed.
- Never touch, focus or stop the owner's game. Only tools/dev-snapshot.ps1 and the shared background fallout2-ce-test.exe are allowed for verification. Check the shared tester is absent first. No test process or build was running at this pause.
- Main checkout was restored to its original main branch bd55fbe7 after an earlier worker cwd error. All subsequent edits are in the gpt worktree. Canonical Foundation files were verified; the prior explicit scoped user exemption remains in force, do not repair Foundation.

## Accepted bounded stages

- c7afe65d: CPU records/capture/copy/restoration and CI suite. Fake buffers cover opaque and transparent copies, source background transport, clipping, overwrite, overlap scrolling, orphan and destroyed generations, and capped records.
- b98e4b8a: live capture bridge. Minimal hooks in font drawing, blits, memory allocation/free/realloc, window storage and SDL storage. OptionsPanel vector storage is explicitly registered; its visible page copies in one rectangular blit. Accepted prefix, original logical width and mono cell origins survive clipping.
- c837badc: CPU sharp text drawing core. Embedded IBM Plex Mono SemiBold; font IDs 103/104 use Bold. CP1251, physical-resolution cached glyphs, width fitting, mono cells, shadows and underline. Compact per-record visibility masks, window provenance/generations, transparent window and cursor masking. Engine accessors expose plain snapshot structs; standalone CPU tests do not depend on SDL or local build/_deps.
- Parent independently reran standalone text_layer + text_capture CTest (2/2) and embedded_fonts CTest (1/1). Worker rebuilt those targets and reported full CTest 14/14. These results apply to the core stage, NOT the unfinished pipeline checkpoint.

## Current unfinished checkpoint

Live presentation integration was interrupted before build or runtime verification. The current checkpoint may NOT compile. Do not call it implemented or accepted.

- New scene_renderer/text_present.{h,cc} prepares temporary restored indexed/RGBA copies, renders a high-resolution overlay and handles failures. Original indexed game buffers must never change.
- context.{h,cc} has a separate sharp overlay texture, preserving the cached base texture used by static scene PNGs. Verify texture unit zero is selected before every texture bind/upload.
- svga.cc has partial live presentation wiring; scene_draw.cc has partial independent PNG rerender wiring. Both must show the same base + sharp image. Preserve transparent world background after restoring index zero, and retain a complete original-frame fallback on failure.
- settings has default-on [3d] sharp_text and an English menu line. Options grouping was adjusted. tools/dev-snapshot.ps1 has opt-in -SharpText, default off for existing regression captures.
- devtools parser currently assigns settings.scene3d.sharp_text for --dev-sharp-text=0/1. Verify initialization order: settingsInit may overwrite the assignment. Renderer must read production settings, not depend on devtools for feature logic.
- Resume by inspecting the whole pending integration diff and fixing compile/link errors. Check svga.cc still declares its existing textCaptureRegisterExternalBuffer / Unregister calls after include changes. Move new render/fallback algorithms out of upstream svga.cc into our modules, retaining only short hooks/calls per AGENTS.md; remove the empty anonymous namespace. Verify settings persistence and accurate menu description for both 2D and 3D.
- Add snapshot metadata for actual sharp activation, captured/drawn runs/glyphs, restored pixels and timing. No metadata claiming a successful sharp pass exists yet.

## Build hazard: do not trust an incremental EXE

Parent found broken MSVC/Ninja include dependency tracking:

    ninja -C build -t deps "CMakeFiles\fallout2-ce.dir\src\devtools\devtools.cc.obj"

reported zero dependencies. rules.ninja has an encoding-mangled Russian msvc_deps_prefix. A changed text_font.h or generated git_version.h may not rebuild callers. The earlier b98e4b8a runtime had an old embedded ce_commit despite a new EXE SHA. A normal configure alone is insufficient: perform a full clean rebuild after integration, with the official MSVC environment, before runtime evidence. Do not turn a local workaround into a broad tooling refactor in task025. Record the build issue for later if necessary.

## Existing runtime evidence and its limit

runs/025-sharp-text/capture-bridge-review/ contains an official background English preferences bottom x2 capture (1920x1080, logical960x540): scr00048.png, JSON and console logs. EXE SHA b38f626a05c65af1fbd4bdadc4db5b2e80672cb2141d8bc1d415da81e71414d1. Parent compared decoded RGBA with task017 final-matrix-20261022/3d-x2-english-bottom/scr00037.png: zero changed pixels. However embedded ce_commit was e29ef058 due broken header deps; this proves that EXE's unchanged picture only, NOT complete fresh capture bridge coverage. No sharp-text-on game image has been produced or reviewed.

## Exact continuation order

1. Finish small presentation integration, compile and clean-rebuild. Preserve standalone CI builds for both text_layer and embedded_fonts. Rerun CTest and record actual fresh EXE hash/source commit.
2. Check no fallout2-ce-test.exe. Take only EN/RU Preferences x2 off/on pairs using official tools/dev-snapshot.ps1 (-Preferences, -Language english/russian, -SharpText only for on, private output dirs). Parent visually reviews one image at a time and checks metadata before expanding coverage.
3. Verify x2/x3 English/Russian across every task Acceptance screen: Pip-Boy, scrolling dialog replies, inventory, character, barter, options, message log, floating text and hover/pressed button labels. Exercise window/cursor occlusion, fade-to-black, original glyph erasure, unknown-buffer pixel fallback. Add bounded dev-only UI fixture helpers if necessary, with real handlers and sandboxed state.
4. Measure actual full Pip-Boy text frame time and memory. Implementation uses cached CPU glyph bitmaps uploaded as a separate high-resolution GL overlay; document this honestly rather than claiming a GPU glyph atlas. Check that it meets the project frame-time target.
5. Release build, full CTest, exporter tests, regression unit tests, and 16 actual regression captures with sharp_text OFF. Do NOT refresh baseline references to hide a failure.
6. Only after all Acceptance holds: curated snapshots + Log, status done, reviewed per AGENTS, PR with required template, CI, review, squash merge. No task025 PR was opened and nothing from025 was merged at this pause.
