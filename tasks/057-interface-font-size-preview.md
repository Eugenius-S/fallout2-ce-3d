---
id: 057
title: Interface font size and weight rollout
milestone: M1
status: doing
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: [056]
reviewed: no
---

## Goal

Apply the owner's selected font-101 appearance globally: scale 1.29 with the embedded IBM Plex Mono SemiBold 600 face. Vertically center the SPECIAL stat-description labels inside their existing content boxes. Keep the original pixel font exact when SharpText is off.

## Context

- The owner selected E=1.29 and SemiBold 600 from the five-size and two-face previews. Keep all preview captures and their original history in this task.
- The actual Russian character screen has 9 logical pixel text rows at 1920x1440 scale 3. The selected scale increases modern glyph coverage within each live owner window; legacy drawing, accepted widths, line height, and wrapping stay unchanged.
- The SPECIAL description field has a 17-row interior in both active character-editor background FRMs. Center only the modern glyph ink for those two tagged draw calls; preserve legacy text coordinates.
- Keep source color behavior from task 056: normal interface green `#3FB419`, lower HUD log green `#4BD51F`, all other palette states unchanged.

## Acceptance

- With SharpText enabled, every eligible font-101 run uses scale 1.29 and SemiBold 600, including UI copies. Other font IDs retain their previous behavior.
- The SPECIAL stat-description role is attached only at its two real font-draw call sites. Its modern glyph ink is centered within the measured 17-row box and its copied records retain the box geometry.
- With SharpText disabled, inventory, dialogue, and character-screen captures are pixel-identical to original rendering.
- Runtime evidence covers Russian inventory, dialogue, character screen, and HUD at scale 3, English barter and a boxed label at scale 2, plus PipBoy. Compare original and final frames for inventory, dialogue, and character.
- Renderer and capture tests cover selected defaults, per-window preview overrides, label centering and copied geometry, glyph clipping, palette fades, and unchanged original palette states.
- Release build, CTest, and the 16-point SharpText-off regression pass.

## Out of scope

- Adding a public size or weight control, changing game-font resources, or modifying layout outside the two SPECIAL description labels.
- Changing the owner-selected palette roles or tinting non-green states.

## Log

### 2026-10-24 gpt
Done: claimed task 057 on the branch from origin/main.
Next: audit a bounded preview-only scale path, then implement after plan review.
Check: branch starts at `d64e12a6`; all tracked files were clean before branching.

### 2026-10-24 gpt
Done: added the opt-in character-screen preview at 1.00, 1.08, and 1.15 in commit `95700dbd`. Release build and CTest pass 14/14. Curated the normal 1.00 control and three variants in `runs/057-interface-font-size-preview/accepted/`. The normal control and explicit 1.00 PNGs decode pixel-identically. Visual review found no adjacent-row collisions or clipped diacritics in the 1.08 and 1.15 frames.
Next: owner chooses a preview size. Keep the production default at 1.00 until that choice.
Check: `runs/057-interface-font-size-preview/accepted/preview-report.md`, the matching snapshot JSON and fixture stdout in each variant folder, and CTest 14/14.

### 2026-10-24 gpt
Done: extended the preview-only allowlist with D=1.22 and E=1.29 in commit `d25c69e6`. Release build and CTest pass 14/14. Both new real character-screen captures use source `d25c69e6`, the same EXE SHA, and the same 1920x1440/x3 fixture setup. Curated the full PNG, snapshot JSON, and stdout under `runs/057-interface-font-size-preview/accepted/preview-1x22/` and `preview-1x29/`.
Next: owner chooses from 1.00, 1.08, 1.15, 1.22, and 1.29. Keep the production default at 1.00 pending that choice.
Check: the two new fixture logs confirm their requested scales; the renderer tests cover all five scales at x2 and x3.

### 2026-10-24 gpt
Done: added a character-fixture-only face override for comparing SemiBold 600 with the existing Bold 700 at E=1.29 in commit `c0bf3b08`. Release build and CTest pass 14/14. Both genuine screen captures use the same EXE SHA, source, and 1920x1440/x3 setup. Curated the original PNGs, snapshot JSON, and stdout under `runs/057-interface-font-size-preview/accepted/preview-1x29-bold/` and `preview-1x29-semibold/`.
Next: owner chooses a size and face. Keep production font 101 at 1.00 Bold 700 until that choice.
Check: `runs/057-interface-font-size-preview/accepted/preview-report.md`; both JSON files and fixture logs confirm scale 1.29, the requested weight, and matching source and EXE SHA.

### 2026-10-24 gpt
Done: owner selected production scale 1.29 and SemiBold 600. Added a semantic box for only the two SPECIAL stat-description draws; confirmed the active creation and edit background FRMs share a 17-row box interior.
Next: finish default, centering, copy, and OFF-pixel tests; rebuild and capture the selected global appearance across the approved UI fixtures.
Check: active resources resolve `edtrcrte.frm` and `edtredt.frm` from `mods/rpu_russian.dat`; their box borders are rows 40 and 58, with content rows 41 through 57.

### 2026-10-24 gpt
Done: set SharpText font-101 fallback to 1.29 SemiBold 600; added copied box geometry and ink-union centering for the two SPECIAL description calls. Added 9/10px x2/x3 centering tests and capture/copy metadata checks.
Next: full clean Release build, CTest, then source-bound UI and original-pixel comparisons.
Check: stock `master.dat` and active mod both show matching 17-row label interiors in `edtrcrte.frm` and `edtredt.frm`; `git diff --check` passes.
