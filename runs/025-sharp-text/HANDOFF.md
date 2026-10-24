# Task 025 handoff, 2026-10-24

## Scope and safety

- The owner's latest correction narrows the feature to small green NPC conversation replies and player answers only. Menus, titles, headers, Pip-Boy and every other UI font stay pixel-rendered.
- Worktree: `E:/Projects/fallout2-ce-3d-gpt`, branch `task/025-sharp-text`. Do not switch branches in `E:/Projects/fallout2-ce-3d` or touch its owner executable or window.
- Only `tools/dev-snapshot.ps1` may start `fallout2-ce-test.exe`. Check for both the shared tester and another worker's launcher before every snapshot. Stop only the tester started by this helper.
- Preserve untracked task017 evidence, old failed task025 captures and local build folders. Stage only explicitly selected evidence files.

## Accepted implementation and evidence

- `80d57da5`: semantic conversation capture tag, restricted to font 101 and runtime `COLOR_GREEN`; original menu text remains untouched.
- `6a4a86ed`: resource-verified `Kcsulik.int` fixture on Klamath, with the engine talk-range check and two distinct live dialog owners.
- `af5163ff`: corrected technical design and palette-fade coverage for CPU-rasterized cached glyph bitmaps and the separate physical RGBA overlay.
- `01b3164f`: curated menu proof and the eight-case NPC matrix. The menu off/on pairs are pixel-identical. NPC pairs cover English/Russian at x2/x3; metadata binds them to source `6a4a86ed` and executable SHA-256 `3173879bcdae46afd9ee328d0e2be5bbe5cc0921e981f46bd6d9dcd86f61412c`.
- Parent independently reviewed the English x2 off/on pair and Russian x3 sharp-on image. The full CTest passed 14/14.

## Current edge-case stage

- `fd886190` adds a read-only reply-offset accessor and a gated `-DialogPageInspect` mode. It starts a real Klamath conversation, records the visible reply text, queues exactly one real engine `KEY_2`, then reports the changed reply text, byte lengths and `dialogReplyTextOffset`. Sharp text must be off for this inspection.
- OFF inspection on `fd886190` confirmed the selected live reply has offset 0 and 61 visible bytes across two text runs: `The spirits with me, fren! They be all around. Sometimes` and `talk.` Evidence log: `runs/025-sharp-text/page-inspect-20261024/fd886190-en-off/process.stdout.log`. No page-down was injected; this is a short reply, not paging evidence.
- REJECTED: the `2af21dea` cursor screenshot at `runs/025-sharp-text/cursor-runtime-20261024/2af21dea-en-x2/` shows the engine cursor still at screen center. Its alpha-mask change only proved a computed mask at the requested location, not that the cursor was drawn there. Preserve this artifact as rejected evidence; do not cite it as cursor acceptance.
- `1c00e08b` fixes the gated `-DialogCursorCheck`: redraw the real engine cursor after moving it, confirm the cursor rectangle and opaque palette index in the indexed framebuffer and prepared frame, then independently decode the snapshot PNG and compare the physical target RGB to the cursor palette color. It also verifies other ink remains in the same reply run.
- Parent independently accepted the corrected fixture at `runs/025-sharp-text/cursor-runtime-20261024/1c00e08b-en-x2-validated/`. The official helper verified target (610,530) in the decoded 1920x1080 PNG as palette RGB (84,84,84); the log verifies overlay alpha 126 to 0 and same-run ink 2601 to 2408. Metadata reports 7 runs, 281 glyphs, 1,812 restored pixels and 2.049 ms preparation at scale 2. EXE SHA-256: `fdd9ed58c09d67c9e101d525b0f3471bfa587943a7574466c32e483dfb41f8f2`.
- The Release build was configured and built through `tools/configure-build.ps1` after `1c00e08b`; `ce/src/platform/git_version.h` embeds `1c00e08b`. Full CTest: 14/14. The helper's PNG decoder is `tools/check_png_pixel.py`.
- Next: parent owns the final 16-case sharp-off regression and CTest. Keep task 025 open until those checks and review finish. CPU capture tests cover clipped scrolling, but no long live NPC reply with a nonzero page offset has been observed, so do not claim runtime paging acceptance. Do not synthesize text, bypass talk rules, or use OS cursor/focus APIs.

## Build and delivery state

- Local Ninja header dependency tracking is unreliable. Use the official configure/build script for MSVC. Rebuild affected source objects after code changes; do not run `ninja -t clean` or remove broad build outputs.
- Draft PR #77 contains the curated matrix and accepted corrected cursor proof; parent runs final regression and status review. Do not mark task 025 done or reviewed before those checks finish.
- Final task scope is the conversation-only Acceptance in `tasks/025-sharp-text.md`; the earlier full-interface plan in old notes is superseded.
