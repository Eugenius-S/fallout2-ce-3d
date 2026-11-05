# Installer layout evidence

Captured 2026-11-05 from the Release `fallout2-ce-3d-launcher.exe` on branch `task/200-installer-layout`.

- Build SHA-256: `65ACDE90B377ED9BAB48AA2448AA22241986C9928F781F0E3D3B31080CC87793`
- Screen: 1000 x 939 pixels, 125% Windows scaling.
- Each JPEG is a desktop pixel capture of the current launcher window. The 10-pixel DWM resize-shadow edge was cropped from the left, right, and bottom.
- English: `installer-en.jpg` (118,773 bytes)
- Russian: `installer-ru.jpg` (121,941 bytes)
- Sidecar: `capture.json` records process IDs, capture coordinates, build identity, and the display-only game path.

A separate launcher process used `--no-activate` and a private settings file for each language. The displayed `C:\Games\Fallout 2` path is a placeholder; no install or update was started. The owner's game was not opened or modified.
