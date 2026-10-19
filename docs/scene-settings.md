# 3D scene settings

The engine settings schema in `ce/src/settings.cc` registers `[3d].enabled` as
a boolean with the default `true`. The setting is stored in
`game/fallout2.cfg`. Missing values use the default. The parser accepts `1`,
`true`, `yes`, and `on` for enabled, plus `0`, `false`, `no`, and `off` for
disabled; matching ignores case and surrounding whitespace. Other values fall
back to `true`. This key-specific rule does not change legacy boolean parsing.

When disabled, gameplay uses plain 2D. When enabled, the Full 3D scene is used
where scene data exists; if a scene is unavailable, gameplay safely falls back
to 2D.

Scene code can read `scene::enabled()` and `scene::settingsRevision()` from
`ce/src/scene/scene_settings.h`; this revision tracks `[3d].enabled` only. Other
features can query `settingsGetRevision(descriptor)` for their own registered
key. Per-setting revisions increase when a config read or applied edit changes
the effective value. The aggregate `settingsGetThreeDRevision()` remains
available for callers that need any `[3d]` change. Revisions are monotonic and
non-consuming, so each feature can retain its own last-seen revision.
`scene::SettingsLifecycle` is a small helper for the scene view's initial
enable and later setting transitions.

In dev mode, Ctrl+F10 remains a view shortcut. If 3D is off, it first turns
`[3d].enabled` on through the registered setting, then selects the Full view.
With 3D enabled, debug view changes remain active until the setting changes.

`tools/dev-snapshot.ps1 -NoPreview` omits the launcher's explicit hero-preview
request. It is useful for checking the ordinary settings-driven startup path;
the default-on setting can still show a scene. Use an isolated config directory
to test `enabled=0` without editing the user's configuration.
