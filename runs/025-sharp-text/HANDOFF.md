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
- The Release build was configured and built through `tools/configure-build.ps1` after `fd886190`; `ce/src/platform/git_version.h` embeds `fd886190`. Executable SHA-256: `8afe14f656d58621b3883c353375f61584c84352acb810851f39c629a5340780`. Full CTest: 14/14.
- Next: once the shared tester is free, run the official helper once with English, `-DialogFixture -DialogPageInspect`, sharp text off and a private output directory. If `reply_offset` is zero, stop and report the actual reply text and lengths. Do not inject page-down unless a nonzero offset proves a paged response. Parent reviews the result before further runtime edge checks.
- Approved follow-up, only after that result: use one real `KEY_ARROW_DOWN` for a proven paged response and move the engine mouse position over an actual reply glyph. Verify cursor masking on the hovered glyph while other glyphs in the same run remain drawn. Do not synthesize text, bypass talk rules, or use OS cursor/focus APIs.

## Build and delivery state

- Local Ninja header dependency tracking is unreliable. Use the official configure/build script for MSVC. Rebuild affected source objects after code changes; do not run `ninja -t clean` or remove broad build outputs.
- Draft PR #77 contains the curated matrix while the bounded edge checks and final regression remain in progress. Do not mark task 025 done, mark the PR ready, or merge it until parent review and all remaining acceptance checks are complete.
- Final task scope is the conversation-only Acceptance in `tasks/025-sharp-text.md`; the earlier full-interface plan in old notes is superseded.
