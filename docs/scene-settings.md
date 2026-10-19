# 3D scene settings

The engine settings schema in `ce/src/settings.cc` registers `[3d].enabled` as
a boolean with the default `true`. The setting is stored in
`game/fallout2.cfg`. Missing values use the default; values other than `0`,
`1`, `false`, or `true` (case-insensitive, surrounding whitespace ignored)
also fall back to `true`. This key-specific rule does not change legacy boolean
parsing.

Scene code can read `scene::enabled()` and `scene::settingsRevision()` from
`ce/src/scene/scene_settings.h`. The revision increases when the effective
setting changes. It is monotonic and non-consuming, so each feature can retain
its own last-seen revision. `scene::SettingsLifecycle` is a small helper for
the scene view's initial enable and later setting transitions.

In dev mode, Ctrl+F10 remains a view shortcut. If 3D is off, it first turns
`[3d].enabled` on through the registered setting, then selects the Full view.
With 3D enabled, debug view changes remain active until the setting changes.

`tools/dev-snapshot.ps1 -NoPreview` omits the launcher's explicit hero-preview
request. It is useful for checking the ordinary settings-driven startup path;
the default-on setting can still show a scene. Use an isolated config directory
to test `enabled=0` without editing the user's configuration.
