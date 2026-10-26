---
id: 073
title: Glow strength ladder: each glowing thing at five strengths for the owner to pick
milestone: M1
status: doing
owner: gpt
suggested: glm
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The owner wants every glow to read clearly ("прям ощутимыми"). They pick the
strength from test shots: each kind of glowing thing shot at five strengths,
1 the faintest to 5 the strongest, side by side.

## Context

- The glow is 022 (emissive buffer, bright pass) and 050 (the wide halo of
  the marked cycling colours); `[3d] glow_strength` scales it, clamped to
  0..3, default 1 (`ce/src/settings.cc`). The ladder may need more than 3 at
  its top: widen the clamp if level 5 calls for it.
- A background run forces the glow off unless `-Glow`; it has no way to set
  the strength yet. Add one (`--dev-glow-strength=X`, a `-GlowStrength X`
  switch in `tools/dev-snapshot.ps1`), as `-Hour` was added for the sun.
- The kinds, at least: a burning barrel and an open fire (Klamath), a
  computer monitor and the vault's screens (`vault15`, live cycling colours
  per 050), the toxic slime (klatoxcv), an energy weapon's bolt if a fixture
  can show one. Night views where the map has them: a glow reads in the dark.
- If the halo and the near glow should scale apart (the owner may want a
  wider halo, not a brighter core), shoot that as a second ladder for one
  kind and say so in the Log.

## Acceptance

- `runs/073-glow-ladder/<kind>.png` for each kind: one sheet with the same
  close view at strengths 1 to 5 in a row, each labelled with its number and
  the `glow_strength` value it maps to; plus the same view with the glow off.
- The owner's pick goes into the Log and becomes the default in
  `settings.h` and `settings.cc` (user-check: yes).

## Out of scope

New glowing things (what is not marked or bright today).

## Log
