# Dev-mode launcher for the sprite-lighting mod (spec edition 2, P1).
#
# Prepares the working copy config (a monitor-sized window at x2, no intro)
# and starts fallout2-ce.exe with the dev tools in the 3D scene. The default
# target is Klamath Downtown (kladwtwn), elevation 0. By default the game
# starts a new game with the first premade character (Restoration Project
# needs a new game); -Slot N loads that save from game/data/SAVEGAME instead.
# -Classic keeps the original 2D picture. The dev panel (Alt+D) travels
# between maps, F9 takes a metadata snapshot, Ctrl+F10 leaves the 3D scene.
param(
    [string]$Map = "kladwtwn",
    [int]$Elevation = 0,
    [int]$Slot = 0,
    [switch]$Classic,
    [switch]$FullScreen
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$GameDir = Join-Path $Root 'game'
$Cfg = Join-Path $GameDir 'fallout2.cfg'

# Dev profile: every game pixel doubled, no intro. By default a framed window
# that fills the primary monitor above the taskbar; -FullScreen covers the
# whole monitor without a frame (no display mode change, Alt+Tab works).
# The game is DPI aware, so sizes are physical pixels.
Add-Type -AssemblyName System.Windows.Forms
$monitor = [System.Windows.Forms.Screen]::PrimaryScreen
$video = Get-CimInstance Win32_VideoController |
    Where-Object { $_.CurrentHorizontalResolution -gt 0 } | Select-Object -First 1
$physicalWidth = if ($video) { [int]$video.CurrentHorizontalResolution } else { $monitor.Bounds.Width }
$physicalHeight = if ($video) { [int]$video.CurrentVerticalResolution } else { $monitor.Bounds.Height }
$dpi = $physicalWidth / $monitor.Bounds.Width
if ($FullScreen) {
    $windowed = '2'
    $width = $physicalWidth
    $height = $physicalHeight
} else {
    # Leave room for the window frame and title bar inside the work area.
    $windowed = '1'
    $width = [int]([math]::Floor(($monitor.WorkingArea.Width * $dpi - 20) / 2) * 2)
    $height = [int]([math]::Floor(($monitor.WorkingArea.Height * $dpi - 80) / 2) * 2)
}
# CE drops the scale when the game picture would be under 640x480.
$scale = if ($width / 2 -ge 640 -and $height / 2 -ge 480) { 2 } else { 1 }
$screen = [ordered]@{ windowed = $windowed; resolution_x = "$width"; resolution_y = "$height"; scale = "$scale" }

$lines = [System.Collections.Generic.List[string]]([System.IO.File]::ReadAllLines($Cfg))
$screenIdx = $lines.FindIndex({ param($line) $line -match '^\[screen\]' })
if ($screenIdx -lt 0) {
    $lines.Add('[screen]')
    $screenIdx = $lines.Count - 1
}
$end = $screenIdx + 1
while ($end -lt $lines.Count -and $lines[$end] -notmatch '^\[') { $end++ }
foreach ($key in $screen.Keys) {
    $found = $false
    for ($i = $screenIdx + 1; $i -lt $end; $i++) {
        if ($lines[$i] -match "^$key=") { $lines[$i] = "$key=$($screen[$key])"; $found = $true }
    }
    if (-not $found) { $lines.Insert($end, "$key=$($screen[$key])"); $end++ }
}
[System.IO.File]::WriteAllLines($Cfg, $lines)

if (-not (Select-String -Path $Cfg -Pattern 'skip_opening_movies=1' -Quiet)) {
    Add-Content -Path $Cfg -Value "[ui]`nskip_opening_movies=1"
}

$toolsCommit = (git -C $Root rev-parse --short HEAD) 2>$null
if (-not $toolsCommit) { $toolsCommit = 'unknown' }

$exe = Join-Path $GameDir 'fallout2-ce.exe'

# Install a newer build. A game that is still open keeps running from its file:
# Windows lets a running exe be renamed, so it is moved aside, not closed.
Get-ChildItem $GameDir -Filter 'fallout2-ce.old-*.exe' | ForEach-Object {
    try { Remove-Item $_.FullName -ErrorAction Stop } catch { }
}
$built = Join-Path $Root 'build\fallout2-ce.exe'
if ((Test-Path $built) -and (-not (Test-Path $exe) -or (Get-Item $built).LastWriteTime -gt (Get-Item $exe).LastWriteTime)) {
    try {
        Copy-Item $built $exe -Force -ErrorAction Stop
    } catch {
        Rename-Item $exe ("fallout2-ce.old-" + (Get-Date -Format 'yyyyMMdd-HHmmss') + ".exe")
        Copy-Item $built $exe
    }
    Write-Output "INSTALLED: $built"
}
$start = if ($Slot -gt 0) { "--dev-load-game=$Slot" } else { '--dev-new-game' }
$args = @('--dev-tools', $start, "--dev-map=$Map", "--dev-elevation=$Elevation", "--dev-tools-commit=$toolsCommit")
if (-not $Classic) { $args += '--dev-walk-preview' }
$mode = if ($FullScreen) { 'full screen' } else { 'window' }
Write-Output ("SCREEN: ${width}x${height} x$scale $mode")
Write-Output ("START: " + $exe + " " + ($args -join ' '))
Start-Process -FilePath $exe -ArgumentList $args -WorkingDirectory $GameDir
