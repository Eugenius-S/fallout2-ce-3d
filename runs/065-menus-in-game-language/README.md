# Menus follow the game language

The shared catalog is `ce/src/scene/menu_text.cc`. It selects Russian for
`[system] language=russian` (case insensitive), otherwise English. Unknown
strings keep their original English fallback. Russian source text is UTF-8;
the catalog converts it to CP1251 for the existing pixel and sharp-text fonts.
The font assets and text renderer are unchanged.

Settings initialization localizes all 13 registered `[3d]` labels and
descriptions. The options panel also translates its title, section headings
and toggle values. The developer panel translates its labels, buttons,
instructions and common status messages; map filenames and renderer IDs stay
identifiers. The Russian map line is shortened to fit the existing panel.

## Evidence

- `options-russian.jpg` and `options-english.jpg`: the full custom options
  panel, cropped from 1920x2160 scale-2 framebuffer captures so all rows fit.
- `devpanel-russian.jpg` and `devpanel-english.jpg`: the developer panel,
  cropped from 1920x1080 scale-2 framebuffer captures.
- `captures.json`: raw image names, resolutions, languages and EXE hashes.
- `regression-summary.json`: 16/16 world scenes pass, 0 changed pixels, against
  the temporary current-base control reference set established in 066.
  Committed references were not updated; their earlier drift remains task 084.

The ordinary helper now captures the developer UI with `-DevPanel`, using the
framebuffer rather than a clean 3D render that omits UI. Its screenshot copier
also resolves vanilla PNGs under `shots/game` when their sidecars are under
`shots/dev`, fixing the existing path mismatch used by these captures.

## Reproduce

```powershell
tools/dev-snapshot.ps1 -Preferences -PreferencesPosition Bottom -ScreenWidth 1920 -ScreenHeight 2160 -Scale 2 -Language russian -CaptureConsole -OutputDirectory build/menus-ru
tools/dev-snapshot.ps1 -Preferences -PreferencesPosition Bottom -ScreenWidth 1920 -ScreenHeight 2160 -Scale 2 -Language english -CaptureConsole -OutputDirectory build/menus-en
tools/dev-snapshot.ps1 -DevPanel -Language russian -CaptureConsole -OutputDirectory build/devpanel-ru
tools/dev-snapshot.ps1 -DevPanel -Language english -CaptureConsole -OutputDirectory build/devpanel-en
```

Release build, 15 CTests, 52 exporter tests and 12 regression unit tests pass.
The settings tests require a Russian label and description for every registered
`[3d]` key, including future additions. They also check real settings
initialization in Russian and back to English, fallback behavior, language
case handling, CP1251 Cyrillic/yo and dynamic filename preservation.

Only isolated background helpers were launched. The owner's game and config
were not installed over, focused or restarted.
