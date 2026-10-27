---
id: 099
title: Point lights next to cut-out pixels no longer get a broken wall normal
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: [098]
reviewed: no
---

## Goal

The point lights (barrels, slime, lamps) light walls the same way next to a
cut-out pixel as anywhere else, as the sun already does since 090.

## Context

- Found in 090 (inbox, 2026-10-26): the point lights' wall normal in
  `sceneLighting` (`uLightMode` 2) takes `dFdx/dFdy(vWorld)` after the alpha
  `discard`. Next to a discarded pixel NVIDIA gives zero derivatives and the
  normal turns NaN.
- 090 fixed the same thing for the sun only; read its fix in the scene
  shader and do the same for the point lights.
- In 090 the same change moved 179 to 1155 px in the caves (klaratcv,
  klatoxcv), so this needs a look check there.

## Acceptance

- before | after sheets of klaratcv and klatoxcv near their lights, and one
  lit Klamath spot (barrels or slime), in `runs/099-point-light-normal-nan/`,
  with the changed pixel count; no new dark or bright specks in the after.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

The sun, the light colours and their strength.

## Log

### 2026-10-27 — point-light face normal

- Reused `gFaceCross`, computed before alpha discard, for scene point lights; no sun, color, or strength changes.
- Targeted 3D before/after: KLARATCV e0 tile 17541: 1,179 changed RGBA pixels; KLATOXCV e0 tile 17941: 929; KLATRAP e0 tile 19721: 4 only with dev test ambient override 16384 (normal ambient=1.0 gives 0). Captures were 1920x1080, scale 2, matching scene/camera metadata. Sheets and enlarged changed-edge crop are in `runs/099-point-light-normal-nan/`; full captures are in ignored `build/099-raw/`.
- Direct final 3D RGBA regression against clean 098 baseline: 10/16 exact. Six Rat/Toxic cave points changed (204, 1,175, 676, 764, 300, 861 px respectively), within intended point-light scenes. Classic `-Still` framebuffer control: 12/16 available points exact 0 px with matching metadata, `scene_view=off`; KLACANYN e0 and KLATOXCV e0/e1/e2 did not emit screenshots within 25s. Not claiming full 16-point Classic pass.
- CTest 18/18 and regression Python unit tests 12/12 pass. Reference checker reports seven failing spots on the fixed build; four already failed on clean 098 baseline (KLAGRAZ e0, KLARATCV e2, KLATOXCV e0/e1). Three additional cave spots now differ due to intended lighting. Direct RGBA attribution and baseline counts are in the evidence report. References were not updated.
- Baseline/fixed EXE SHA256: `23a382f0d445f924bba10acdd4e58e6839f112b637803d0a79398d532ad1984c` / `4f2ce48a9aa7dedfd0f12e0dce07bd226f8ece6b3e56f7f5e4b7edf415c3ca9f`.
