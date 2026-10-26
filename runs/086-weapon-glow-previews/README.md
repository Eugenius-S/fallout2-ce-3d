# 086: native effect emission, local light and glow

## White suppression and chosen effect calibration

white-projectiles.jpg, white-fire-electric-rocket.jpg, white-explosions.jpg
and white-signs.jpg compare the previous chosen look (left) with reduced white
bloom (right). The compositor retains 15% of the neutral RGB component after
blur and keeps its chromatic remainder. Applying this after blur also catches
white created by adjacent colored source pixels mixing. The scene picture,
self-lit source cores and local illumination are not dimmed.

The owner's selected former common strengths are now internal per-kind
calibration at the unchanged COMMON default 2, with one player slider:

| Kind | Selected former strength |
| --- | --- |
| Laser, plasma pistol/rifle, electricity | 0.7 |
| Legacy plasma bolt | 1.5 |
| Flamethrower, rocket in flight | 1.0 |
| All four explosions | 0.7 |

The RGBA8 emission target clamps before blur. The calibrated material therefore
reproduces that clamp before applying selected_strength / 2; scaling its raw
gain before the clamp would produce a different look. Signs have no final pick:
their white-suppression comparison provisionally uses 0.4 in both columns.

Raw captures: build/effects086/white-balanced. tools/effect_white_review.py
validates matching source/camera state, pixel-identical OFF images versus the
previous build, reduced added neutral light for all 13 examples, and source
hashes in white-measurements.json. The runtime build and 16 CTests pass;
64 exporter tests and 12 regression tests pass. All 16 ordinary views matched
the controls before the final effect-only clamp correction; the final 13 OFF
comparisons additionally verify unchanged sources and surface lighting.

## Earlier calibration ladders

The earlier outcome is night-projectiles.jpg, night-fire-electric-rocket.jpg,
night-explosions.jpg and night-signs.jpg. Each row compares the owner's requested
COMMON glow strengths 0.7 / 0.8 / 1.0 / 1.3 / 1.5 / 1.8, left to right. Source compensation and
local lights are identical across columns; the global default remains 2.
No extra player controls were added. The accepted PLASMAB bloom material stays
unchanged; the other known native FX get self-lit hot cores and compensation.
Smoke and rocket casing remain ordinary material. Known SIGN40/41 red/yellow
tubes use the same core selection, including baked geometry via its runtime
object table. Scene file formats are unchanged.

night-signs-low.jpg adds the owner's sign-only comparison at 0.3 / 0.4 / 0.5.
It uses the same crops, executable and lighting as the six-column sign sheet.
sign-low-measurements.json verifies identical previous OFF pixels and scene
state. Reassemble with `py -3 tools/effect_glow_levels.py --sign-low` from raw
captures in build/effects086/sign-low. This changes previews only.

All 13 examples use one executable, 1920x1080, scale 2, ambient 16384 and fixed
clock 0. Eleven effect fixtures use KLADWTWN at 25075, hero 25280, solo radius 1.
The two real DENBUS1 signs retain their native placements: SIGN40 tile 16274,
hero on the same tile to open its roof, zoom 2; SIGN41 tile 17080, zoom 1.
The sign images include their actual surroundings. Sources and camera state,
matching frozen scenes, pixel-identical OFF controls across the two batches,
increasing response and hashes of the raw captures
are validated in glow-levels-measurements.json by tools/effect_glow_levels.py.
Raw images stay in build/effects086/tuning and tuning-low and are not committed.

The visual lights follow live object tiles, pixel offsets and current frames;
hidden/missing objects stop submitting them. They use the native wall/door
spread on an object COPY without modifying CE's hex light map or object flags.
Synthetic FX reserve slots within the existing 32-light budget and do not
invalidate static-light spreads as their positions change. Native lights are
not duplicated. The existing lighting switch disables surface illumination;
the single glow slider controls bloom independently of source visibility.

Verification: Release game/mapper, 16 CTests, 64 exporter tests, 12 regression
unit tests. All 16 ordinary views equal the current-base controls (0 changed
pixels); an additional roof-change smoke view also matches. Eleven fixtures
pass move/offset/hide/restore/lights-off plus native-map/field preservation.
Before/after surface probes with bloom OFF show added light outside every
effect's sprite (lighting-probes.json, captured before the roof-only correction).
These are native static fixtures with movement checks, not
a full combat replay. The owner's running game and config were not changed.

This is branch evidence pending visual calibration and integration with newer
main (task 087 sprite-camera directions). It is not an installed game build.

Reproduce the requested strengths, replacing name/frame from the measurements:

```powershell
tools/dev-snapshot.ps1 -Map kladwtwn -ViewTile 25075 -HeroTile 25280 -Solo 1 -EffectFixture laser -EffectFrame 5 -EffectLightCheck -GlowLevels 1.3,1.5,1.8 -Ambient 16384 -Hour 0 -CaptureConsole -OutputDirectory build/effects086/tuning/laser
tools/dev-snapshot.ps1 -Map kladwtwn -ViewTile 25075 -HeroTile 25280 -Solo 1 -EffectFixture laser -EffectFrame 5 -EffectLightCheck -GlowLevels 0.7,0.8,1 -Ambient 16384 -Hour 0 -CaptureConsole -OutputDirectory build/effects086/tuning-low/laser
py -3 tools/effect_glow_levels.py
```

## Historical daylight inspection

The descriptions below refer only to the initial sheets, before the production
emission/local-light changes. review-*.jpg also preserve that earlier material's
OFF/2/4/8 exploration; they are not the current nighttime result.

Four sheets show eight native misc effects. In every row: glow OFF on the
left, the accepted common strength 2 on the right. These are static native
frames in the real object/card renderer, not generated art or a combat replay.

These initial sheets use FULL ambient light (65536), fixed clock 0, scale 2,
1920x1080. They inspect native color and the existing bloom independently of
night attenuation. They DO NOT prove the new requirement for self-emission
and colored local light in darkness. That implementation and night/sign
comparisons remain pending. No production emission or lighting was changed.

| Sheet | Effects |
| --- | --- |
| laser-and-bolt.jpg | Laser pistol projectile, legacy plasma bolt asset |
| plasma-weapons.jpg | Plasma pistol, plasma rifle |
| fire-and-rocket.jpg | Fire explosion, rocket explosion |
| plasma-and-emp.jpg | Plasma explosion, EMP explosion |

sources.json records real FIDs, asset filenames, selected frames/directions,
frame counts and cycling-pixel counts. The representative frame maximizes a
simple bright-pixel score. Frame labels are one-based; JSON indices are zero-based.
The legacy plasmab.frm is a native asset, not a claim about every weapon's
projectile choice. Normal laser/pistol/rifle prototype references were inspected.

The fixture creates a no-save native object, without combat/damage, extra point
lights, or saving. Projectiles use direction 1 to show their length. A fixture-only
fly camera centers its real sprite card and fits each effect independently;
relative size between different effects is therefore not a gameplay comparison.
Camera, native frame and all scene metadata stay identical within every pair.

measurements.json validates both states, matching effect FID/frame/direction,
matching scene state, and a nonzero crop response. Native screen_rect is the
engine's 2D rectangle, not the fitted fly-camera rectangle; camera_eye/yaw/pitch
record the actual fixture camera. All sheet crops are (600,200)-(1320,880),
reduced equally into 360x340 cells. No exposure or color adjustments.

Reproduce, substituting each sources.json name/frame:

```powershell
tools/dev-snapshot.ps1 -Map kladwtwn -ViewTile 25075 -HeroTile 25280 -Solo 1 -EffectFixture laser -EffectFrame 5 -GlowPair -Ambient 65536 -Hour 0 -CaptureConsole -OutputDirectory build/effects086/final/laser
```

Checks: Release game/mapper build, 15 CTests, 64 exporter tests and 12 regression
unit tests pass. All 16 ordinary views match 066 current-base controls exactly;
old committed references remain unchanged (their drift is task 058).
Owner executable/config/process were not installed, restarted or modified.
