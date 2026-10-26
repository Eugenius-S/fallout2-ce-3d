---
id: 086
title: Self-lit weapon effects, local colored light and luminous sign previews
milestone: M1
status: done
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: [073]
reviewed: yes
---

## Goal

Implement the owner's three layers for native energy/fire effects: self-lit
cores, colored local illumination and bloom controlled by the one accepted
global strength (default 2). Show night comparisons and two real luminous signs.

## Context

The owner requested more effects after approving 073's monitor and slime.
The developer harness has no weapon-effect fixture yet. Use native misc art,
the real object/card renderer and 073's corrected frozen palette.

## Acceptance

- A bounded developer fixture places a native effect at a chosen tile/frame,
  without combat, damage, saving or touching the owner's game.
- Same-scene OFF/default-2 pairs for available laser/plasma and fire, plasma,
  EMP and rocket explosions, labelled with real asset/frame information.
- Native plasma/laser/electrical/flamethrower/explosion/rocket cores remain
  self-lit at night, cast colored light and have bloom; keep one global slider.
- Two real luminous sign night comparisons, with verified source assets.
- JPEG sheets under runs/086-weapon-glow-previews, at most 1 MB per file.
- Build/tests and ordinary glow-off regression remain green against controls.

## Out of scope

Weapon balance, damage/trajectory changes, new art, extra per-kind menu
controls, or changing the approved global glow default.

## Pipeline

INPUT: real native effect objects/frames and verified luminous sign assets.
TRANSFORM: classify only actual emitters and select hot source pixels consistently.
TRANSFORM: self-lit material, colored local-light contribution, existing bloom pass.
MEANING: source cores stay bright in darkness; nearby surfaces receive their color.
OUTPUT: all three layers, controlled by existing global lighting/glow settings.
DIRECT CHECK: night source/surface/halo probes, frame lifetime and ordinary-view controls.
The native fixture, production emission/local lights, accepted calibration and
two night sign examples are complete and integrated with main. Final source
checks, fresh-export regressions and the full main-versus-branch fly tour pass.

## Log

### 2026-10-26 gpt: initial inspection complete, owner expanded scope
Done: native no-save effect fixture in devtools/weapon_effect_fixture.{cc,h},
-EffectFixture/-EffectFrame and generic -GlowPair. Actual object/card renderer,
fixed cycle palette, fitted camera, OFF/default-2 pairs. Eight pairs and four
JPEG sheets in runs/086-weapon-glow-previews. No gameplay renderer change yet.
Check: Release game/mapper; all 15 CTests, 64 exporter, 12 regression unit tests.
All 16 normal views equal 066 current-base controls, 0 changed pixels.
Owner game/config were not restarted, installed, focused or modified.

New owner requirements: plasma, laser, electricity, flamethrower fire,
explosions, rockets and similar real effects must have THREE layers: intrinsic
emission, colored light on nearby surfaces, bloom. Add two actual luminous
sign examples. Keep one global glow slider/default 2. This supersedes the
original emission-mask exclusion; the task remains doing.

Verified implementation gaps:
- scene_objects.cc marks only palette indices >=229. Ordinary laser/plasma
  pistol/rifle, electrical/fire/explosion art mostly uses other indices and
  darkens with ambient. Daytime bloom is not proof of self-emission at night.
- scene_lights.cc collectLight requires native intensity/OBJECT_LIGHTING;
  actions.cc provides projectile/explosion light only when optional content
  emit_light is enabled (default false in item.cc). Do not rely on it.
- findEmitter rejects uncycled sprites whose RGB sum is <150/189; this misses
  saturated red laser texels. A shared emitter selection must serve material,
  bloom and local-light color/location; do not make all bright scenery emit.

Proposed bounded continuation:
1. Extend the fixture for elect.frm (misc 14), fthro.frm (26), rocket.frm (9).
2. Shared native effect classification and hot-core mask. Preserve dark smoke
   and unlit rocket casing; no blanket whitening of every nonzero sprite pixel.
3. 3D-only colored lights with native wall/door spread. Avoid persistent CE
   object/light-map mutations and remove transient light when its effect hides
   or dies. Preserve the 32-light cap; avoid invalidating every static-light
   spread on each projectile frame. Existing scene_lights.cc has a shared cache.
4. Use existing material/baked-object flags where practical; inspect the
   scene renderer's wall-row table before inventing a scene-file format.
5. Find two genuinely luminous signs (not painted boards), night shots and
   source/surface/bloom checks at glow 2. One candidate: denbus1 e0 tile17080,
   SIGN41.FRM, prototype intensity65536; still verify visually. Klamath/Den
   scan found no second qualified sign. Native renosgn1..9 assets exist:
   search their map instances for the second example; export only own cache.
6. Recheck ordinary views and production frame lifetime; current daytime
   sheets are explicitly preview-only and do not satisfy these new criteria.

Next: owner budget/context decision. Per the owner's economy instruction,
warned that real emission + dynamic-light work materially expands the request
and costs more tokens in this long chat. Save/push this state and wait for
further guidance before implementing the new production-lighting scope.
Active worktree: E:/Projects/fallout2-ce-3d-gpt-065, branch task/086-weapon-glow-previews.

### 2026-10-26 gpt: emission/local-light implementation and 1.3/1.5/1.8 review

Owner explicitly resumed implementation and confirmed the existing repository
must be continued without repeating Foundation setup. PLASMAB at glow 2 is the
accepted reference; compensate the other FX internally with one global slider.
Then the owner requested all examples at 1.3 / 1.5 / 1.8. Default stays 2 until
the visual choice is made; these sheets are experimental compensation values.

Done: effect_emission.h classifies verified native FX and SIGN40/41, selects
hot pixels for material and local-light color/position, and excludes smoke and
rocket casing. scene_objects.cc/shader tag self-lit cores with per-kind bloom
compensation. Runtime baked sign flags reuse the wall-object table without
changing scene file formats. scene_lights.cc adds visual sources for effects
without native CE lights, respects wall/door spread using a source copy, avoids
double lighting and reserves the existing 32-light budget. Moving synthetic FX
do not invalidate the static spread cache. Native CE fields/light map stay put.
Roof blending now erases hidden emission instead of leaking a sign's glow.

Fixture: added electric/flamethrower/rocket and two existing DENBUS1 signs.
GlowLevels captures OFF plus any 1..8 requested levels in one frozen process.
NoEffectEmission provides a comparison control. EffectLightCheck verifies the
actual submitted source through tile/pixel movement, hiding/restoring and
lighting off, with unchanged native hex light map and object light fields.
Snapshot metadata now reports lighting correctly (previously hardcoded off).

Evidence: four night-*.jpg sheets, columns 1.3 / 1.5 / 1.8 for all 11 effects
and two real signs. glow-levels-measurements.json validates 52 captures from
one build, identical state within rows, exact strengths and increasing glow.
lighting-probes.json records before/after floor changes with bloom disabled
for every effect, plus movement/lifetime checks. Earlier review-*.jpg are the
daytime pre-implementation ladders already present when this run resumed.

Check: Release game/mapper, 16 CTests, 64 exporter and 12 regression unit tests.
All 16 normal views equal current-base controls (0 changed pixels); an extra
entrance view after the roof change also matches. Runs size and diff checks
pass. Owner game/config were not installed, restarted, focused or modified.

Next: inspect the requested visual levels and finalize compensation/default.
Before PR/merge integrate origin/main (now c88f4062, includes task 087 sprite
camera directions and task 058 refreshed references), rebuild and rerun the
current-main checks. This branch has not yet integrated those newer changes.
Keep the task doing; production implementation is saved, not merged/deployed.

### 2026-10-26 gpt: append 0.7/0.8/1.0 to the same review layouts

Owner requested three lower common strengths alongside the previous values.
Done: 52 additional night captures for all 11 FX and both signs, using the
same executable, scene parameters, source gains, lighting and camera as before.
Each of the four night sheets now has six columns:
0.7 / 0.8 / 1.0 / 1.3 / 1.5 / 1.8. Default 2 and production code unchanged.
tools/effect_glow_levels.py verifies both batches, pixel-identical OFF controls
for every example, matching build/state and strictly increasing glow response.
Check: all 13 six-level comparisons validate; Python compilation, diff and runs
size checks pass, each JPEG under 1 MB. Prior runtime checks still apply because
only the evidence assembler, images, measurements and documentation changed.
Next: visual calibration and the main-integration step described above.

### 2026-10-26 gpt: lower sign-only preview

Done: requested SIGN40/41 comparison at 0.3 / 0.4 / 0.5 in night-signs-low.jpg.
Eight frozen captures use the previous build, cameras, source gains, surface
lights and crops. sign-low-measurements.json records matching previous OFF
pixels, matching scene metadata and increasing bloom response. Reproduce with
tools/effect_glow_levels.py --sign-low after sign-low captures.
Check: both sequences, Python compilation, diff and runs size checks pass.
Production values and the owner's other pending visual picks are not applied.
Next: finish visual calibration, then integrate main as described above.

### 2026-10-26 gpt: suppress white bloom and fold in the owner's FX picks

Owner resumed after observing that white bloom overwhelms the source details.
Final picks from chat: laser/plasma pistol/plasma rifle/electric 0.7, legacy
plasma bolt 1.5, flamethrower/rocket 1, all explosions 0.7. Electricity's earlier
3 was explicitly corrected to 0.7. No final sign pick; compare those at 0.4.

Done: post.cc splits blurred bloom into neutral min(R,G,B) and its chromatic
remainder, retaining 15% of the neutral share in both bloom scales. The scene
picture is composited with full share 1, so intrinsic emission and local lights
remain unchanged. Suppression happens after blur, also catching neutral white
created by mixing neighboring colored source pixels. One global slider remains.

effect_emission.h and scene_shader.cc encode the selected FX looks at common
default 2. Calibration reproduces the reviewed RGBA8 target's clamp BEFORE
selected_strength/2 scaling; scaling the raw gain first would change the look.
PLASMAB retains its cycling-only core mask through an explicit cycling field.
Signs keep their previous per-kind material while their selection is pending.

Evidence: white-projectiles.jpg, white-fire-electric-rocket.jpg,
white-explosions.jpg, white-signs.jpg, previous chosen look left/new result right.
tools/effect_white_review.py validates all 13 source/frame/camera states,
pixel-identical OFF images against previous captures, and lower added neutral
light for every example. Raw captures: build/effects086/white-balanced.
white-measurements.json records source hashes, executable hashes, added neutral
values and near-white pixel counts. Surface illumination is unchanged exactly.

Check: Release game/mapper, 16 CTests, 64 exporter and 12 regression unit tests.
All 16 ordinary views matched current-base controls during white implementation
(white-regression-summary.json); the final correction only changed calibrated
FX tags and was verified by the final 13 OFF comparisons. Diff, Python and runs
size checks pass. Owner game/config have not been installed/restarted/modified.
Next: inspect the new white suppression, finalize signs and integrate the newer
main before PR/merge as above. Task stays doing; this is saved branch work.

### 2026-10-26 gpt: accepted result, integrated main and final verification

Owner accepted the right-hand white-suppressed result including the weaker EMP
as a tradeoff, and authorized completing the task and merging the PR. Signs use
the shown 0.4 equivalent internally at COMMON default 2. There is one slider.

Done: merged main ea4bba3f without conflicts, preserving 087 sprite directions.
SceneObjectArt/shownObjectArt is shared by renderer and emitter extraction, so
turned effects cast their light from their actual visible frame/side. Live and
baked signs use calibrated material 245/runtime wall flag 3. No scene-file
formats, owner game files or live game process were changed.

Final evidence: final-*.jpg (OFF with source/local light vs common glow 2),
final-measurements.json validates 13 pairs from the integrated build and 11
move/offset/hide/restore/lighting-switch/native-map preservation checks.
Source fixture captures preceded the final cache refresh; they use unchanged
native art and valid scene caches. The authoritative world checks below use
fresh exports, not the old borrowed junction targets.

Check: Release game and mapper, all 16 CTests, 64 exporter and 12 regression
unit tests. Fresh own Klamath export replaces the borrowed map junctions while
holding the background test mutex; borrowed targets and game mod-cache remain
untouched. All 16 regression views match the current committed main references
with ZERO changed pixels (final-regression-summary.json). The initial stale
cache comparison was rejected, not used as final evidence.

Full fly tour against a separately built main ea4bba3f, both on the same fresh
cache: 43 spots, three angles, 129 views per build, 0 of 43 spots changed.
changed.jpg and final-tour.json record that required review. Windows PowerShell
module lookup was repaired for the tour subprocess only, without system edits.
All four final JPEG sheets and changed.jpg are below 1 MB; checks and review
pass. No reference snapshots were retaken and no serialized format changed.

Next: GitHub CI, then squash-merge this PR. Installation/combat testing in the
owner's primary game is a separate step; it has not been launched or installed.
