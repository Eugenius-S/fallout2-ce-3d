# Muzzle flash captures

These are isolated, frozen native-art frames at night, at rotations 1 and 4.
The projectile in the energy comparisons is positioned at the painted muzzle
for inspection; this is not a replay of a projectile in flight. Each batch
uses the same scene, camera, animation and build for its comparison rows.
The player lamp is off to show local illumination from the effect.

- Bullet weapons: flash and its light, then the same frames without them.
- Energy and other projectiles: native glow, then a small additional muzzle
  flash. For a weapon without a separate projectile, the first row uses its
  native flame or painted flash as the light source, and the middle row shows
  it without the added light. The owner selected the extra flash for laser and
  plasma weapons with a separate projectile.
- The single global glow setting controls the result. Frames and rotations
  are labelled in the images. SMG frames 1, 3 and 5 are its painted flashes.

Native prototype inventory used for the comparisons: laser pistol 16 and rifle
118 use lazrpis.frm; plasma pistol 24 and rifle 15 use plazpis.frm; pulse
pistol 396 and Alien Blaster 120 use elect.frm; pulse rifle 397 and Gatling
Laser 28 have no separate projectile. Flamer 11 uses its own firing frames;
rocket launchers 13 and 270 use rocket.frm and robtrock.frm; Dual Plasma Cannon
498 uses pball2.frm; End Boss Plasma Gun 531 has no separate projectile.
Other variants using the same firing style and projectile are represented by
these rows. Lightsabers are melee weapons, not firing animations.

Rebuild from the no-save snapshot batches with `py -3 tools/muzzle_sheets.py`.
