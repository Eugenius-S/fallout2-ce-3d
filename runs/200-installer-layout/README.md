# Installer layout evidence

Captured 2026-11-05 from the Release `fallout2-ce-3d-launcher.exe` on branch `task/200-installer-layout`.

- Build SHA-256: `986A4656D3DC1DDD896661D960EC4D7AB45D2321AB243A1128D4C80E2BEACB88`
- Launcher window: 816 x 759 pixels; evidence crops the 10-pixel DWM edge from the left, right, and bottom.
- Each JPEG is a fresh `PrintWindow` rendering of the current launcher window handle.
- English: `installer-en.jpg` (67,876 bytes)
- Russian: `installer-ru.jpg` (69,161 bytes)
- Sidecar: `capture.json` records language, process ID, build identity, and capture method.

A separate launcher process used a private settings file for each language. The visible install path is a display-only placeholder; no install or update was started. The owner's game was not opened or modified.
