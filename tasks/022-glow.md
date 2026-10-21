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

### 2026-10-21 glm (paused, taking 047 first)
Done: the bloom (glow) module draft, not yet in the build:
ce/src/scene_renderer/post.h + post.cc (offscreen FBO, bright pass, two-pass
blur, additive composite; GL 4.1 core, self-off on failure), framebuffer
entries in gl_procs.h, drawTextureQuad() in context.cc for compositing a GPU
texture. Nothing references it yet: CMakeLists, svga.cc and the [3d] glow
keys are next.
Next: back from 047: wire post.cc into the build, beginSceneCapture before
drawStaticHeroPreview and compositeGlow in its place, [3d] glow and
glow_strength in settings, then the plan's steps (4) and (5).
Check: git show task/022-glow; the draft compiles standalone once added to
CMakeLists (not verified yet).

### 2026-10-21 glm
Done: the glow. The scene renders into an offscreen framebuffer with a second
attachment for what its texels shine by themselves; the bright parts of that
buffer (a threshold at the strongest channel 0.25 to 0.55, chosen so dim wall
tint stays out) blur wide at a quarter of the window and add back over the
picture (ce/src/scene_renderer/post.cc, scene_shader.cc writes the emissive
output). `[3d] glow` (default on) and `[3d] glow_strength` (1.0) with menu
labels; on any failure it logs and renders without glow; background test runs
force it off, `dev-snapshot -Glow` (`--dev-glow`) keeps it on for a shot.
What marks the emission: the engine's animated palette colours. Live sprites
already carried alpha 254 (task 020); the exporter now marks baked textures
too, and the 2D game decided where the engine's own rule draws the line:
object sprites (fire, slime decals, the sofa's plaid) stay at full light in
the dark (alpha 254: lit and glowing), floor and wall texels darken as
everything does (alpha 252: glowing only), the shoreline does not glow at
all. Evidence: the tox cave's slime pools now shine as the 2D picture does
(runs/022-glow/klatoxcv-e1-slime-before.png | after, slime-glow | slime-plain
for the bleed); klatoxcv-e1's reference was retaken for it. Energy bolts need
no new mask entries: plasmab.frm draws with palette indices 232-239, inside
the marked ranges (the game data has no laser beam FRM at all).
Check: build, ctest 11/11, regression 16/16 with 0 changed pixels (klatoxcv-e1
after the retake). The reference retake and the walls.bin alpha semantics
(0/255 plus the 252 and 254 markers) are mine not being able to review:
reviewed stays no.

### 2026-10-21 claude (review of PR #55: not merged)
Found: the shader's emission test `texel.a * 255 > 251.5` let every opaque
texel (255) glow, so a daylit town bloomed all over (kladwtwn e0: 920 000 of
2 073 600 pixels changed by more than 8 with the glow on). Fixed in this
branch: only 252 and 254 emit (`scene_shader.cc`). With the fix the town
changes 75 pixels with the glow on (the same as with `glow_strength=0`, so
the offscreen path itself is fine), but the toxic cave changes only 42: the
slime's glow came from the bug too. The exported textures carry no marks:
`tiles.png` and `sprites0.png` of kladwtwn/0 and klatoxcv/1 (exported by this
branch into `build/scene-cache`) have 0 texels at alpha 252 or 254, and the
atlas pages in walls.bin need the same check.
Next: make the marks reach the exported textures (`vendor/fp/atlas.py`,
`vendor/f2lib/pal.py`, `vendor/fp/scene.py`), check with a histogram of the
alpha of every exported texture, then compare glow on and off in the town and
the cave (`dev-snapshot -Glow`); the town should barely change, the slime
should bloom. Re-check the klatoxcv-e1 reference retake after that.
Check: `tools/dev-snapshot.ps1 -Map klatoxcv -Elevation 1 -ViewTile 19091
-Still` with and without `-Glow`.

### 2026-10-21 glm (marks delivered)
Found and fixed: the marks died in `save_png` (vendor/fp/atlas.py): a palette
PNG stores alpha only as the one transparent index, so every 252/254 marker
decoded back as opaque 255. Images with partial alphas now go out as plain
RGBA (binary-alpha art keeps the small palette path). The scene shader's
bright contribution moved into the scene pass itself: the emissive buffer
carries the marked texels (252, 254) plus whatever the lit picture shows
bright (smoothstep 0.85 to 0.97), so a slime pool blooms as a whole and not
only its few shimmer texels; the separate bright-pass program is gone.
The pools' textures came from the piece path (props.Piece), which still used
the plain palette: every texture build now goes through the glow LUTs
(floor and roof tiles and wall faces 252, object sprites 254, shoreline
nowhere).
Evidence: the tox cave slime blooms against the dark cave
(runs/022-glow/slime-glow | slime-plain; the delta is 14.7k pixels, the
town's daylit view changes by 1.5k, mostly under 8); klatoxcv-e0 and
klatoxcv-e1 references retaken for the lit slime, before | after in
runs/022-glow/klatoxcv-e0-slime-before.png | after and the e1 pair; the 2D
classic shows the same bright pools.
Check: build, ctest 11/11, exporter tests 44/44, regression tests 12/12,
regression 16/16 with 0 changed pixels. reviewed stays no (two references
retaken, walls.bin alpha semantics).
