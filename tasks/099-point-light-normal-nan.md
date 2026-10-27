---
id: 099
title: Point lights next to cut-out pixels no longer get a broken wall normal
milestone: M1
status: todo
owner: none
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
