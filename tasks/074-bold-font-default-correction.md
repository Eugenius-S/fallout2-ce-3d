---
id: 074
title: Use Bold 700 and refine SharpText UI text
milestone: M1
status: done
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: [057]
reviewed: yes
---

## Goal

Use the owner's corrected font-101 default weight: IBM Plex Mono Bold 700. Keep the selected global size at 1.29 and the main inventory/PipBoy size at 1.22. Fix the reported font treatment in injury labels, PipBoy underlines, and floating character barks.

## Context

- The previous task applied SemiBold 600 based on an earlier selection. The owner has now explicitly corrected that choice to Bold 700.
- Keep the Bold/SemiBold preview override available for future comparison.
- Preserve all shipped size scopes, SPECIAL-label centering, plaque lifts, dialogue spacing, colors, and SharpText-off pixels.
- The owner reports inactive injury labels too dark, PipBoy underlines misaligned with the larger glyphs, floating character barks retaining pixel outlines, and live-owner text appearing smaller than reviewed captures. The first three are bounded rendering fixes. The live-owner size difference is investigated as a source/config mismatch and must not trigger an unrequested restart.
- Live-game audit on 2026-10-25: owner process was `E:\Projects\fallout2-ce-3d-glm\game\fallout2-ce.exe`, SHA-256 `1C7233B89C249D987BD2B40B834A14E5600102447659991EEE19AD29D2EBFB8B`, GLM commit `e08785c8`; its config was 2310x1298, scale 2, SharpText enabled. It predates the merged font implementation. Current approved captures use the task build at 2310x1298, scale 2; no owner process was changed.

## Acceptance

- Eligible font-101 runs use Bold 700 by default at 1.29, except tagged main-inventory and PipBoy runs, which use Bold 700 at 1.22.
- Explicit per-window face preview overrides still select either embedded Bold or SemiBold.
- Inactive, unselected injury labels use `COLOR_DARK_GREEN`; selected, active, and non-green text retain their existing colors.
- Modern PipBoy underlines follow the visible glyph ink bounds with a one-logical-pixel gap and matching horizontal span; SharpText-off pixels remain unchanged.
- Modern floating character barks render their outline from the sharp glyph mask and restore the legacy morphology pixels during replacement/removal; SharpText-off remains unchanged.
- Live-owner comparison records executable hash, source, resolution, and scale, without closing, focusing, or restarting the owner's game.
- Fixtures and tests report/assert the effective Bold default while retaining explicit SemiBold override coverage.
- Release build, CTest, Python exporter/regression suites, and SharpText-off regression pass without updating references.
- Source-bound inventory, PipBoy, character, and dialogue captures show the requested Bold default; original font pixels remain unchanged when SharpText is off.

## Out of scope

- Changing font sizes, dialogue spacing, legacy coordinates, public settings, or font resources.

## Log

### 2026-10-25 gpt
Done: claimed the correction task on `task/074-bold-font-default-correction` from `origin/main` at `8489e442`.
Next: change the selected face default to Bold 700, update focused assertions and fixture output, then build and capture.
Check: task `074` is the first free ID after `073-glow-ladder`; no remote task/074 branch exists.

### 2026-10-25 gpt
Done: selected Bold 700 by default, added modern-only injury-status color role, aligned PipBoy underlines to visible glyph ink, and added floating-bark outline capture, restoration, and padded-layout support. SharpText-off source colors, legacy outlines, line placement, and object placement remain on the original path.
Next: add and build the bounded bark lifecycle fixture, capture injury/PipBoy/bark evidence at 2310x1298 scale 2, then run the final SharpText-off regression and close remaining runtime evidence.
Check: clean official MSVC Release build passed all 733 targets; CTest passed 15/15. Focused tests cover injury palette fades, underline span/gap/clipping at E1.29 and D1.22 with x2/x3, exact outline-pixel restoration/copying, and multiline bark layout.

### 2026-10-25 gpt
Done: added a bark-only dev fixture using real CP1251 Russian bark strings. It captures the longer outlined bark, replaces it with the shorter line, verifies visible font-101 outline records through owned window/generation visibility masks, then captures the cleared frame after expiry. The fixture logs visible pixel counts.
Next: review the narrow fixture diff, build it, and validate SharpText-on/off lifecycle captures.
Check: no own `fallout2-ce-test.exe` process was active before runtime work; the owner GLM process remains untouched.

### 2026-10-25 gpt
Done: first bark runtime attempt exposed a fixture setup omission: `textObjectAdd` callers refresh the exact returned tile rectangle, while the fixture had passed no rectangle and never requested a redraw. Updated both bark adds to refresh the returned rectangle and changed `textObjectAdd` to return `FloatingTextOutlineLayout::renderBounds`, which includes the modern-only padded backing extent. Legacy layout returns its original rectangle. Extended the scene test to assert the full refresh rectangle.
Next: rebuild and rerun the bark fixture; then capture the final character, PipBoy, and bark screens from this source.
Check: first attempt reached `UI_FIXTURE START` but made no snapshots and ended at the harness deadline; no result was accepted from that run.

### 2026-10-25 gpt
Done: identified that the active 3D Full-view presentation returns before `vanillaPresented`, so the bark-only lifecycle stage never advanced. Added a bark-gated fixture callback after the scene overlay swap and a temporary capturing stage to prevent reentrant snapshots; ordinary fixtures and modal/dialog hooks are unchanged.
Next: rebuild, confirm the real bark appears and is removed by the engine, then capture final source-matched character/PipBoy/bark images.
Check: second attempt also ended without snapshots at the isolated harness deadline; it is diagnostic only. Only the task's `fallout2-ce-test.exe` PID was stopped, after confirming its exact worktree executable path.

### 2026-10-25 gpt
Done: the first bark frame showed the Russian line in the rendered scene. Its 707-pixel count came from raw copied records and is not accepted as visibility proof. The first replacement attempt timed out. Added scene-module refresh bounds that accept the renderer's `tileSceneShift` for add and removal rectangles, plus a shifted-bounds unit assertion.
Next: rebuild with a bounded diagnostic for actual scene shift, old/new copied-run counts, and object count; cap replacement at 300 frames and natural expiry at 1200.
Check: replacement has not passed yet. The prior attempt used the pre-shift build, so it does not establish whether the shift caused the timeout.

### 2026-10-25 gpt
Done: added the requested replacement diagnostic and separate 300-frame replacement / 1200-frame expiry bounds. The first bounded run exposed a startup ordering bug: the scene callback could count frames before the bark fixture entered its first active stage. It now ignores Idle and Finished stages.
Next: rebuild and rerun once the isolated tester queue is clear.
Check: that run emitted `UI_FIXTURE FAIL` without a `START` line, so it is not lifecycle evidence.

### 2026-10-25 gpt
Done: review found a fixture reentrancy error: during `tileWindowRefreshRect`, the stage still said `WaitingForBark`, so nested scene callbacks re-added the replacement repeatedly. The 3/63 record counts came from that broken harness and are invalid. The fixture now switches to `WaitingForBarkReplacement` before mutation/refresh and suppresses nested callbacks for that operation. It counts only visible font-101 records with matching window owner/generation and nonzero visibility masks. Removed the unnecessary owner-count API.
Next: rebuild and inspect the single replacement frame, then verify natural expiry and a clear frame.
Check: prior diagnostic builds showed a scene shift of 0,0; replacement and cleanup remain unverified.

### 2026-10-25 gpt
Done: source `b2b11252` passes the bark lifecycle fixture at 2310x1298 scale 2: the long Russian bark has 1955 visible mask pixels; the short replacement has 595 and `old_visible_runs=0`; expiry leaves `cleared_runs=0`. The wrapper accepted all three source-matched sidecars after its CRLF-aware PASS check. Character and PipBoy fixtures also passed at this source; their pixel hashes match the earlier accepted frames. Full-size source-bark images, sidecars, stdout and EXE SHA-256 are preserved under `runs/074-bold-font-default-correction/runtime-b2b1/`.
Next: run the full SharpText-off 16-spot regression without updating references, then finish task evidence and PR.
Check: official Release incremental build and CTest passed 15/15 at source `b2b11252`; owner GLM game was not focused, stopped, or restarted.

### 2026-10-25 gpt
Done: started the SharpText-off regression without updating references. The first three spots (`entrance`, `corrals`, `residents`) passed with 0 changed pixels, each from source `b2b11252` and EXE SHA-256 `b42652371149fd364a573aba284cf1497cdf267118d88a18056148fabd3ce704`.
Next: rerun the full 16-spot regression after the other worktree's test process releases the global slot.
Check: this attempt stopped at `central-doors` when the harness detected another `fallout2-ce-test.exe`; it is incomplete and not counted as a full regression pass. No other process was stopped.

### 2026-10-25 gpt
Done: the complete SharpText-off regression passed all 16 spots with 0 changed pixels and no reference updates.
Next: merge through root.
Check: run `runs/regression/20261025-144410-708-33728`; every capture sidecar identifies source `b2b11252` and EXE SHA-256 `b42652371149fd364a573aba284cf1497cdf267118d88a18056148fabd3ce704`. The other worktree's tester had exited before launch; no process was stopped.

### Final runtime evidence

- All UI captures: 2310x1298, scale 2, source `b2b11252`, EXE SHA-256 `b42652371149fd364a573aba284cf1497cdf267118d88a18056148fabd3ce704`; JPEG presentation crops and matching metadata are under `runs/074-bold-font-default-correction/accepted/README.md`. Raw PNGs are local under `E:\Projects\fallout2-ce-3d\game\shots\raw-task074\`.
- Inventory and dialogue source-matched captures pass at this final source. Inventory logs 34 font-101 runs with effective `D1.22`; the Russian dialogue fixture reports 11 eligible font-101 runs at the global `E1.29` default. The wrapper's final PNG-copy step used the wrong folder for these two captures; each PNG was retrieved from the exact image path named in its matching JSON sidecar.
- Character injury color: `runs/074-bold-font-default-correction/accepted/crops/character-injury.jpg` (raw PNG SHA-256 `BF269D316F5D8A53156A1C199444F0F4A6786B0C879950509BD96C1DA3EC7C32`).
- PipBoy underline: `runs/074-bold-font-default-correction/accepted/crops/pipboy-underline.jpg` (raw PNG SHA-256 `60987C39333853CD184B6A0E35C430EC841C2E8ED9B3CBD1386EEF55A1EE571A`).
- Bark lifecycle sidecars and logs are under `runs/074-bold-font-default-correction/accepted/bark/`; raw PNGs are in the local `game/shots/raw-task074/` folder. The replacement crop is `runs/074-bold-font-default-correction/accepted/crops/bark-replacement.jpg`. The fixture verified the old bark is absent from the replacement's visible mask and both fixture barks are absent after expiry.
