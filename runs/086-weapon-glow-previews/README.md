# 086: native weapon effect previews, first stage

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
