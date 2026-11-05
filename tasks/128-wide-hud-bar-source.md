---
id: 128
title: Wide-screen art, the HUD bar and the 4:3 full-screen art, with licenses
milestone: M1
status: blocked
owner: gpt
suggested: gpt
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

Know which Fallout 2 mod or patch ships the wide bottom interface bar on the
owner's screenshot, where to get it, and whether its license lets us ship
or reuse that art.

## Context

- Part C of the UI batch (193): taken together with 193 in one PR.
- Batch: the wide art (2026-11-04 inbox sweep). Step 2 folded in, the
  inbox line of 2026-10-27: the 4:3 full-screen art (main menu, splash and
  loading screens, death and ending slides) extended to 16:9 and 21:9. The
  owner's method: regenerate the art wider from the original, downscale to
  the game's pixel size, dither to match, then lay the untouched original
  over the middle with a small gradient mask at its edges. Step 1 (the HUD
  bar source and license) comes first; step 2 is its own PR.
- The owner, 2026-11-01: `runs/inbox/2026-11-01-wide-hud-bar.webp`. The stock
  bar sits in the middle; metal side panels with a round vault-gear ornament
  fill the space to each screen edge.
- Likely candidates: the High Resolution Patch's wide interface art, sfall
  based packs, Fallout 2 Restoration Project, Fallout Et Tu. Check each.

## Acceptance

- A short report in the Log: the source (name, link, version), the files
  that hold the art (FRM names and sizes), the license or the author's terms
  quoted briefly, and a verdict: we may ship it, we may only load it from the
  player's own install, or we may not use it.
- No art committed to the repo.

## Out of scope

Putting the bar into the game. That is a new task after the verdict.

## Log

### 2026-11-01 gpt
Done: The screenshot matches the Mash High Resolution Patch UI family: the
owner's Fallout 2 install has `f2_res.dat` (903,780 bytes), `IFACE_BAR_WIDTH=800`,
and `f2_res.ini` selects metal side art 6. The archive contains
`HR_IFACE_800.FRM` (800x99, 79,274 bytes) plus seven left/right side pairs.
The selected frames are `HR_IFACELFT6.FRM` and `HR_IFACERHT6.FRM` (each
640x100, 64,074 bytes). Pairs 1, 3, 4, and 5 are 640x99 and 63,434 bytes
each; pairs 2 and 7 are 640x100 and 64,074 bytes each. These values were read
from the installed DAT; no art was extracted into the repository or committed.

Evidence and limits: the screenshot's central bar and side panels correspond
in style to those HRP resources and the installed config selects side-art 6.
I rendered the three extracted FRMs with `COLOR.PAL`: the central bar matches
the screenshot's layout and the two side frames have the same dark ribbed
metal, fasteners, and round/grilled motifs. Exact pixel alignment and crop
against the screenshot are not established. The exact
HRP release that supplied this local archive is not identified by its files;
the ini says only “High Resolution Patch for sfall v5.x”. The upstream HRP
source repository is [mattwells77/Fallout-High-Resolution-Patches](https://github.com/mattwells77/Fallout-High-Resolution-Patches),
and the companion resources project is [BGforgeNet/f2_res](https://github.com/BGforgeNet/f2_res).
The HRP 4.1.8 readme documents the 800-pixel art and side-art choices
[here](https://falloutmods.fandom.com/wiki/Fallout_2_High_Resolution_Patch);
this is a community mirror, not the installed archive's version proof.
The open HRP code repository has an MIT license, but that does not establish
rights for binary FRM artwork. The HRP guide attributes the 800-pixel bar to
Continuum and its first metal sidebar to Simon Rawlins (Pixote), and says
“Mod makers feel free to replace or modify them for your own needs.” It does
not clearly grant redistribution of the artwork. RPU's primary readme credits
included graphics to several artists, including Pixote and Continuum, without
a separate redistribution grant:
[RPU readme](https://github.com/BGforgeNet/Fallout2_Restoration_Project/blob/master/docs/rp-readme.txt).
The author/permission for side-art variant 6 is not established from the
available attribution.

Verdict: rights to ship or use the FRMs are not established. Do not bundle,
load, or otherwise use them in this project unless the applicable terms are
verified and permission is obtained from the relevant rights holders. Exact
HRP release, variant-6 author, and explicit art redistribution permission
remain unknown. No art was committed.
Check: inspected `f2_res.ini` and parsed member sizes/frame geometry in the
installed `f2_res.dat` with the repository's read-only DAT/FRM parsers.
At that time: variant-6 provenance and redistribution terms were unresolved;
the final conservative verdict is recorded below.

### 2026-11-04 gpt: license and release decision
Done: checked Mash's Fallout 2 High Resolution Patch readme (versions 4.1.7/4.1.8) against the installed archive. It says mod makers may include the patch and replace or modify included art, asks that it not be sold or passed off as the author's own, and recommends retaining the readme. It credits `HR_IFACE_800.FRM` to Continuum. The author's current GitHub repository labels its MIT license for the patch source code; that license does not establish rights to separately redistributed FRM artwork. The local archive is 903,780 bytes and calls itself the HRP for sfall v5.x, but its exact release is not identified. The stock HRP documentation covers side variants 1 and 2; the installed variant 6 and its author/source remain unverified.
Verdict: do not ship or use the combined bar and variant-6 side art. The documented permission does not establish that this downstream variant-6 artwork is covered. Keep the original files only in the player's existing installation. No art was committed.
Check: inspected the installed `f2_res.ini` and DAT/FRM entries; checked the HRP author's source repository and its MIT license, and the HRP 4.1.8 readme copy. The exact local archive release and variant-6 provenance remain unresolved and are stated as such.
Next: implementation of any wider HUD art is a separate task and must use newly authored assets or art with verified redistribution terms.

### 2026-11-05 gpt: close the source and rights step
Done: the task's acceptance is complete in PR #184: the likely HRP source and FRM dimensions are identified, and the decision is not to ship or load the artwork because redistribution rights and the installed variant-6 provenance are unverified. No art was committed.
Next: the separate wide-screen art implementation belongs to Part C of task 193, after that batch's own requirements; it does not reopen this source-and-rights task.
Check: PR #184 is merged; the source, file list, license limits, and verdict are recorded above.

### 2026-11-05 claude
Step 1 (the HUD bar source and its rights) done above, merged as #184. The
task stays open for step 2 (the 4:3 art) and the art itself, inside the UI
batch (193 part C).

### 2026-11-05 gpt: Step 2 blocker
Done: the source and rights review is complete in PR #184. The identified HRP art is not safe to ship or load because redistribution rights and the installed variant-6 provenance remain unknown.
Next: Part C of task 193 needs newly authored art or a verified authorized source for the wide HUD and 4:3 screens.
Check: PR #184 is merged; the decision and evidence are recorded above.
Blocked: the only identified art source has unresolved redistribution rights, and the required new art is not present in the task files or this worktree. Keep the task open until an authorized art source is available.
