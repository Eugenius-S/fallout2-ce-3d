# Font samples for the sharp text layer

`fonts-ru.png`, `fonts-en.png`: the game's `FONT1.AAF` (Russian from
`mods/rpu_russian.dat`, English from `master.dat`) at x2, then candidates at
x2 resolution, each sized to the game's line height (10 game pixels) and no
wider than the game's text.

All candidates are under the SIL Open Font License, from
<https://github.com/google/fonts/tree/main/ofl>: PT Sans, Roboto, Fira Sans,
Rubik, Sofia Sans, Exo 2, IBM Plex Mono, JetBrains Mono.

Rebuild (fonts downloaded into one folder):

```powershell
py -3 runs/024-sharp-text-design/font_sheet.py . <fonts folder> runs/024-sharp-text-design
```
