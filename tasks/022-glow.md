---
id: 022
title: Glow on fire, screens, signs and energy weapons
milestone: M1
status: doing
owner: glm
suggested: gpt
claude-only: no
user-check: yes
depends-on: [002, 014]
reviewed: no
---

## Goal

Things that shine in Fallout 2 glow: fire, monitors, alarms, signs, laser and
plasma shots.

## Context

- The game marks most shining pixels already: the palette's cycling colors
  (indices 229-254 in `ce/src/cycle.cc`: slime, shoreline, slow fire, fast
  fire, monitors, alarm). Decide per range which glow (shoreline likely not).
- Sprite frames become RGBA card textures in the bridge; floor and wall
  textures come from the exporter. The glow mask has to be made where the
  palette index is still known: at both places.
- The same colours stay at full light in the engine's darkening. Sprite cards
  already mark them (alpha 254, task 020); floor and wall textures from the
  exporter do not yet, so in caves and at night they dim there: the
  exporter's mask fixes that too.
- Energy weapon shots and explosions: check which colors their art uses; if
  they are not cycling colors, add their palette colors to the mask list.
- Post effects stay within OpenGL 4.1 core.

## Acceptance

- A glow (bloom) pass: bright parts bleed light softly; colors keep cycling
  as in the game.
- Klamath: fires, monitors and signs glow; a laser shot glows (test on any
  map with a laser fight, snapshots in the PR).
- `[3d] glow` (default on) and `[3d] glow_strength` with menu labels.
- Regression snapshots with glow off unchanged.

## Out of scope

Light that glowing pixels cast on surroundings (task 020 covers light).

## Log

### 2026-10-20 glm
Done: claimed; researched the implementation surface. No code yet.
Verified facts for the implementer:
- Sprites: ce/src/scene/scene_objects.cc spriteAlphaFor(colorIndex) already
  marks color indices >= 0xE5 with alpha 254 on card textures, and
  ce/src/scene_renderer/scene_shader.cc:174 treats alpha 254 as full-bright
  (ignores the scene light). Energy shots go through the same path; verify
  their FRM palette indices offline with the vendor f2lib (frm.rgba lookup
  uses palette.rgba[index], so the index is the FRM byte value).
- Floors/roofs/walls: the exporter converts tiles via
  vendor/f2lib/pal.py Palette.rgba (256x4, alpha 0 for index 0 else 255) and
  vendor/f2lib/frm.py frm.rgba(palette, ...) - the palette index is known
  exactly there. tools/scene_export/floor_sidecar.py and wall_sidecar.py
  pack the textures; add a parallel per-atlas cycle-index map (R channel =
  palette index, 0 = none) written next to the textures and listed in
  scene.json.
- Live render flow (svga.cc ~520-555): drawStaticHeroPreview() renders the
  scene straight to the default framebuffer, then the game's 2D frame is
  masked (scene::maskOverlay) and drawn over it by drawSceneOverlay, then
  swap. For the bloom pass: bind an offscreen RGBA8 FBO + depth renderbuffer
  at the drawable size before drawStaticHeroPreview, extract bright pixels
  (luminance threshold plus the emissive marker), blur (two-pass separable,
  small downsampled targets, GL 4.1 core), and composite scene + glow additively
  where drawSceneOverlay drew before. Keep the FBO code in scene_renderer
  (context or a new post module), guarded: on FBO/shader failure log and
  render without glow (AGENTS.md failures rule).
- Full-bright for floor/wall emissive texels: extend the scene shader's
  floor/wall paths with the cycle-index texture (or alpha 254 if the
  exporter marks the texture alpha instead - walls already use alpha 0 for
  transparent texels, so 254 fits if the shader treats 254 as emissive and
  255 as plain opaque).
- Palette cycling: CE animates cycles in ce/src/cycle.cc (ranges in
  scene_lights.cc:63: slime 229-232ish, shoreline, slow fire, fast fire,
  monitors; scene_lights.cc glows (index >= 229 && <= 247) || index == 254
  and skips shoreline). The live engine palette (directDrawGetPalette)
  already holds the current phase per tick; re-upload only the atlas cells
  that contain cycling pixels (exporter writes the per-cell bounding boxes
  into scene.json) when the palette tick advances, via TexSubImage2D.
Next: implement in this order - (1) exporter cycle-index maps + C++ load,
(2) shader full-bright for floor/wall emissive texels, (3) bloom pass +
[3d] glow (default on) and [3d] glow_strength with menu labels and
self-off-on-failure, (4) regression/dev-snapshot glow-off switch so
captures stay byte-stable, (5) snapshots 2D classic vs 3D glow in
runs/022-glow/ (fires + a monitor), laser FRM palette-index evidence, PR.
Check: build + ctest; regression with glow off unchanged; fires glow in the
snapshot.
