---
id: 153
title: The sky does not glow or bleed onto the map in flight
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

In the fly camera the sky keeps 142's dusty, muted colour and stays a
background: no bright saturated yellow, and no orange halo bleeding from
the sky over the map's edges and roofs.

## Context

- The owner, 2026-11-02, live on main `3bd28ea5` (just after 142, #204),
  Klamath in flight: `runs/inbox/2026-11-02-sky-glows.webp`, "the sky has
  started to glow in the 3D view". The sky reads bright lemon yellow, far
  from the beige in `runs/142-desert-sky/horizon-no-ridges.jpg`, and a soft
  orange glow spreads from it over the map's edges, the far roofs and the
  corn.
- The owner's game runs with the glow on (`--dev-*` captures force it off,
  so 142's evidence did not show this). Likely cause: `drawSceneSky` draws
  into the scene capture (`scene_draw.cc`, `beginSceneCapture` /
  `compositeGlow` in `post.cc`), so the glow pass takes the bright sky as an
  emitter and adds its bleed, which both brightens the sky and halos the
  map. Confirm with `-Fly -Glow` in the background run, then fix it by a
  rule (the sky emits no glow: drawn outside the glow source, or masked out
  of it), not by darkening the palette.

## Acceptance

- before | after in flight with the glow on and off, at 08:00, 12:00,
  17:30 and 23:00, in `runs/153-sky-glows/`: the sky matches 142's colour
  with the glow on, no halo on the map; glowing things (fire, signs,
  energy weapons) still glow.
- ctest passes; the regression stays pixel for pixel (the game view draws
  no sky).

## Log

- 2026-11-02 Cut from the owner's live find after #204.

- 2026-11-02 gpt
  Done: `sky.cc` now writes an explicit zero to MRT attachment 1 for every sky fragment. Its color output remains at location 0, with the palette unchanged; the scene object's emission shader is untouched.
  Evidence: `runs/153-sky-glows/before-after-glow-on.jpg` and `before-after-glow-off.jpg` show matched Klamath views at 08:00, 12:00, 17:30 and 23:00. With glow off, before and after are pixel-identical at all four times. With glow on, the top 150 rows changed at all 288,000 pixels before the fix and at zero pixels after it. A same-coordinate comparison of the on-minus-off image delta, after subtracting its broad sky component with an 8-pixel Gaussian high-pass, correlates with the baseline at 0.995, 0.973, 0.918 and 0.999 across those times; MAE is 0.19, 0.362, 0.519 and 0.051 per channel. CTest `effect_emission_tests` passes and no object-emission code changed.
  Evidence: `runs/153-sky-glows/after-glow-objects.jpg` compares a fire-explosion fixture at the same map tile, frame 3/7 and camera pose with glow off and on. The glow-on pass changes 64,060 pixels, with maximum RGB delta 144.
  Evidence: `runs/153-sky-glows/fly-tour-glow/changed.jpg` consolidates the glow-on fly tour against the clean `origin/main` build. Klamath elevation 0 passed 57 views across 19 spots; all 19 spots show the expected sky or halo change.
  Visual caveat: all screenshots above were captured from the pre-rebase build with CE sidecar commit `5a0569a2`, based on `origin/main` `bc13d860`. After rebasing onto `0a2322fa`, the Release build and automated tests were rerun, but no runtime screenshot was captured from the rebased HEAD.
  Limitation: `sign40` was not present at the selected Klamath fixture tile 20690. `plasma-bolt` has one frame in this build's art and the attempted frame 3 capture did not run, so sign and energy-weapon glow are not independently verified. Keep the PR draft until those classes have supported fixture captures.
  Next: capture a sign and energy-weapon glow pair with supported map objects or fixture frames, then reassess this limitation.
  Check: Release build passed; CTest 18/18 passed; scene exporter tests 88/88 passed; regression unit tests 12/12 passed; full regression 16/16 spots passed with zero changed pixels. Klamath and seven regression maps were exported only to this worktree's `build/scene-cache`.

- 2026-11-02 claude
  Reviewed and merged: the sky writes zero emission, and the matched Klamath captures show no sky glow after the fix with glow-off frames unchanged. Sign and energy-weapon glow were not captured separately; the change does not touch object emission.
