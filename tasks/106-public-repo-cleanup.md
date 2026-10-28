---
id: 106
title: The public repository holds only the project, no personal setup or paths
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

The repository is public. A visitor sees the game, its tools, its docs and
its evidence, and nothing of the owner's own machine: no personal agent
setup, no user names, no local paths.

## Context

- `.foundation/project.yml` is the owner's local agent-system marker
  (`scope`, `source_mode`, `visibility`). Only task 005 ever read it. The
  owner's tooling still looks for it in the checkout, so it stays on disk but
  leaves git.
- `.codex/config.toml` lists the owner's personal MCP servers to switch them
  off. That is private setup, not the project's.
- Found on main, 2026-10-27:
  - `runs/017-options-menu-3d-panel/owner-brief.md` line 7: a path with the
    owner's Windows user name;
  - `runs/091-launcher-updates/run1-install.log`, `run2-update.log`,
  `run4-update-c091.log`: local checkout paths;
  - `runs/010-scene-per-map/tour2/devhero-20261019-124706-000.jpg`: the byte
    match is in the encoded image data; the file has no personal metadata.
- `tools/` top level has one-off scripts from past tasks
  (`round5_sheet.py`, `muzzle_sheets.py`, `effect_*`, `glow_ladder_sheets.py`
  and the like).
- `runs/` weighs 322 MB on main. History is not rewritten here.

## Acceptance

- `.foundation/` and `.codex/` are out of git (`git rm --cached`) and in
  `.gitignore`; the files stay in the owner's checkout.
- `git grep -il` on main finds no owner's Windows user name, drive path,
  personal account identifier or unapproved e-mail. Text files get the path replaced with a
  neutral one (`<game>`, `<worktree>`); an image with personal metadata is
  re-saved without it. The grep commands and their empty output go in the
  Log.
- A check in CI (next to `tools/check_runs_size.py`) fails a PR that adds a
  personal name, drive path, account identifier or unapproved e-mail.
- One-off scripts in `tools/` that nothing on main runs or names (no other
  script, doc, workflow or open task's `Check`) are removed; the list goes
  in the Log. A script still named somewhere stays.
- Build, tests and regression pass; nothing in `runs/` beyond the files above
  changes.

## Out of scope

Rewriting git history (the 322 MB and old commits stay; the owner decides
that separately), slimming `runs/`, `AGENTS.md` and `CLAUDE.md` (they are
how the project is worked on and stay public).

## Log

### 2026-10-28 gpt
Done: Removed the local agent setup from Git and ignored both local directories. Replaced only absolute local paths in text with `<game>`, `<worktree>` or `<scratch>`. The build setup now derives installation roots from system environment variables. Removed unused one-off scripts `tools/check_png_pixel.py`, `tools/effect_review_sheets.py` and `tools/round5_sheet.py`; retained scripts still named by task checks or run documentation.

Image check: Inspected metadata in 890 run images; no personal metadata was found. The byte hit in the named JPEG is inside encoded image data. `jpegtran` is unavailable; no image was rewritten. The image and all other image pixels remain unchanged.

Checks:
- `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release`: PASS, 761/761 targets.
- `ctest --test-dir build --output-on-failure`: PASS, 18/18.
- Scene exporter unit tests: PASS, 64/64.
- Regression unit tests: PASS, 12/12.
- Public content scanner tests: PASS, 6/6.
- Scene export: PASS, 1,467 resources.
- `py -3 tools/check_public_content.py origin/main`: PASS.
- Runtime regression: 15/16 match. Only `klagraz-e0` differs by 132 pixels, the existing baseline deviation confirmed on clean main during task 099. No reference was changed. Isolated outputs are in `build/106-regression-20261028-084002-711-13468/`; the temporary `runs/regression/` output was moved there after the check.

Grep commands and results:
```text
git grep -I -l -i '[j]iraf' -- .
(empty)
git grep -I -l -i '[i]cons8' -- .
(empty)
git grep -I -l -i '[Y]andexDisk' -- .
(empty)
git grep -I -l -E '[A-Z]:\\' -- .
(empty)
git grep -I -l -i -E '[A-Z0-9._%+-]+[@][A-Z0-9.-]+\.[A-Z]{2,}' -- . ':!ce/third_party/**' ':!**/third_party/**' ':!**/vendor/**' ':!**/LICENSE*' ':!**/COPYING*' ':!**/NOTICE*'
(empty)
```
The unfiltered email search returns only preserved upstream attribution files: `ce/third_party/fpattern/include/fpattern/fpattern.h`, `ce/third_party/fpattern/src/fpattern.c`, `ce/third_party/sdl2/LICENSE` and `ce/third_party/zlib/LICENSE`. They remain byte-for-byte unchanged and are excluded from the owner-data email check in grep and CI.

Changed `runs/` text files (95):
- `runs/003-regression-snapshots/floor-disabled/capture.log`
- `runs/004-gl41-core-context/core41/capture.log`
- `runs/017-options-menu-3d-panel/narrow-frame-20261022/save-1280x960/process.stdout.log`
- `runs/017-options-menu-3d-panel/owner-brief.md`
- `runs/017-options-menu-3d-panel/preferences-probe-20261022/final-evidence/cancel-fixed/process.stdout.log`
- `runs/017-options-menu-3d-panel/preferences-probe-20261022/final-evidence/cancel-normalized/process.stdout.log`
- `runs/017-options-menu-3d-panel/preferences-probe-20261022/final-evidence/defaults/process.stdout.log`
- `runs/017-options-menu-3d-panel/preferences-probe-20261022/final-evidence/escape-normalized/process.stdout.log`
- `runs/017-options-menu-3d-panel/preferences-probe-20261022/final-evidence/save/process.stdout.log`
- `runs/017-options-menu-3d-panel/preferences-probe-20261022/final-evidence/verify-defaults/process.stdout.log`
- `runs/025-sharp-text/capture-bridge-review/process.stdout.log`
- `runs/025-sharp-text/cp1251-extents-20261024/d1677a9b-en-x2-cursor-on/process.stdout.log`
- `runs/025-sharp-text/cp1251-extents-20261024/d1677a9b-ru-x3-dialog-on/process.stdout.log`
- `runs/025-sharp-text/cursor-runtime-20261024/1c00e08b-en-x2-validated/process.stdout.log`
- `runs/025-sharp-text/cursor-runtime-20261024/2af21dea-en-x2/process.stdout.log`
- `runs/025-sharp-text/cursor-runtime-20261024/361694e0-en-x2/process.stdout.log`
- `runs/025-sharp-text/cursor-runtime-20261024/5a8c0183-en-x2-transparent-verified-v2/process.stdout.log`
- `runs/025-sharp-text/npc-dialog-20261024/committed-en-x2-off-6a4a86ed/process.stdout.log`
- `runs/025-sharp-text/npc-dialog-20261024/committed-en-x2-on-6a4a86ed/process.stdout.log`
- `runs/025-sharp-text/npc-dialog-20261024/committed-en-x3-off-6a4a86ed/process.stdout.log`
- `runs/025-sharp-text/npc-dialog-20261024/committed-en-x3-on-6a4a86ed/process.stdout.log`
- `runs/025-sharp-text/npc-dialog-20261024/committed-ru-x2-off-6a4a86ed/process.stdout.log`
- `runs/025-sharp-text/npc-dialog-20261024/committed-ru-x2-on-6a4a86ed/process.stdout.log`
- `runs/025-sharp-text/npc-dialog-20261024/committed-ru-x3-off-6a4a86ed/process.stdout.log`
- `runs/025-sharp-text/npc-dialog-20261024/committed-ru-x3-on-6a4a86ed/process.stdout.log`
- `runs/027-3d-settings-per-key/README.md`
- `runs/054-small-interface-font/accepted-20261024/en-x2-barter-on/process.stdout.log`
- `runs/054-small-interface-font/accepted-20261024/en-x2-character-on/process.stdout.log`
- `runs/054-small-interface-font/accepted-20261024/en-x2-dialog-on/process.stdout.log`
- `runs/054-small-interface-font/accepted-20261024/en-x2-inventory-off/process.stdout.log`
- `runs/054-small-interface-font/accepted-20261024/en-x2-inventory-on/process.stdout.log`
- `runs/054-small-interface-font/accepted-20261024/en-x2-pipboy-on/process.stdout.log`
- `runs/054-small-interface-font/accepted-20261024/ru-x3-character-on/process.stdout.log`
- `runs/054-small-interface-font/accepted-20261024/ru-x3-pipboy-on/process.stdout.log`
- `runs/055-bold-interface-font/accepted-20261024/ru-x3-character-bold/process.stdout.log`
- `runs/055-bold-interface-font/accepted-20261024/ru-x3-character-original/process.stdout.log`
- `runs/055-bold-interface-font/accepted-20261024/ru-x3-dialog-bold/process.stdout.log`
- `runs/055-bold-interface-font/accepted-20261024/ru-x3-dialog-original/process.stdout.log`
- `runs/055-bold-interface-font/accepted-20261024/ru-x3-inventory-bold/process.stdout.log`
- `runs/055-bold-interface-font/accepted-20261024/ru-x3-inventory-original/process.stdout.log`
- `runs/056-interface-text-colors/accepted-20261024/en-x2-barter-on/fixture.stdout.log`
- `runs/056-interface-text-colors/accepted-20261024/ru-x3-character-off/fixture.stdout.log`
- `runs/056-interface-text-colors/accepted-20261024/ru-x3-character-on/fixture.stdout.log`
- `runs/056-interface-text-colors/accepted-20261024/ru-x3-dialog-off/fixture.stdout.log`
- `runs/056-interface-text-colors/accepted-20261024/ru-x3-dialog-on/fixture.stdout.log`
- `runs/056-interface-text-colors/accepted-20261024/ru-x3-inventory-off/fixture.stdout.log`
- `runs/056-interface-text-colors/accepted-20261024/ru-x3-inventory-on/fixture.stdout.log`
- `runs/056-interface-text-colors/accepted-b99d12c3-20261024/en-x2-barter-on/fixture.stdout.log`
- `runs/056-interface-text-colors/accepted-b99d12c3-20261024/ru-x3-character-off/fixture.stdout.log`
- `runs/056-interface-text-colors/accepted-b99d12c3-20261024/ru-x3-character-on/fixture.stdout.log`
- `runs/056-interface-text-colors/accepted-b99d12c3-20261024/ru-x3-dialog-off/fixture.stdout.log`
- `runs/056-interface-text-colors/accepted-b99d12c3-20261024/ru-x3-dialog-on/fixture.stdout.log`
- `runs/056-interface-text-colors/accepted-b99d12c3-20261024/ru-x3-inventory-off/fixture.stdout.log`
- `runs/056-interface-text-colors/accepted-b99d12c3-20261024/ru-x3-inventory-on/fixture.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/barter-en-x2-on/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/character-en-x2-boxed-label/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/character-off/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/character-on/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/compact-inventory-off/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/compact-inventory-on/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/compact-pipboy-off/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/compact-pipboy-on/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/dialogue-layout-off/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/dialogue-layout-on/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/dialogue-layout-refactor-off/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/dialogue-layout-refactor-on/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/dialogue-off/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/dialogue-on/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/inventory-off-repeat/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/inventory-off/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/inventory-on/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/inventory-weight-off/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/inventory-weight-on/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/final/pipboy-on/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/normal-control/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/preview-1x00/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/preview-1x08/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/preview-1x15/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/preview-1x22/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/preview-1x29-bold/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/preview-1x29-semibold/process.stdout.log`
- `runs/057-interface-font-size-preview/accepted/preview-1x29/process.stdout.log`
- `runs/074-bold-font-default-correction/accepted/README.md`
- `runs/074-bold-font-default-correction/accepted/bark/adapter-dfc5ae76/process.stdout.log`
- `runs/074-bold-font-default-correction/accepted/bark/process.stdout.log`
- `runs/074-bold-font-default-correction/accepted/character/process.stdout.log`
- `runs/074-bold-font-default-correction/accepted/dialog/process.stdout.log`
- `runs/074-bold-font-default-correction/accepted/inventory/process.stdout.log`
- `runs/074-bold-font-default-correction/accepted/pipboy/process.stdout.log`
- `runs/080-injury-text-mid-green/accepted/README.md`
- `runs/080-injury-text-mid-green/accepted/process.stdout.log`
- `runs/091-launcher-updates/README.md`
- `runs/091-launcher-updates/run1-install.log`
- `runs/091-launcher-updates/run2-update.log`
- `runs/091-launcher-updates/run4-update-c091.log`
