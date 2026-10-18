# Dev-mode launcher for the sprite-lighting mod (spec edition 2, P1).
#
# Prepares the working copy config (windowed, no intro) and starts
# fallout2-ce.exe with the dev tools. The default target is Klamath
# Downtown (kladwtwn), elevation 0. By default the game starts a new game
# with the first premade character (Restoration Project needs a new game);
# -Slot N loads that save from game/data/SAVEGAME instead. The dev panel
# (Alt+D) travels between maps, F9 takes a metadata snapshot.
param(
    [string]$Map = "kladwtwn",
    [int]$Elevation = 0,
    [int]$Slot = 0
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$GameDir = Join-Path $Root 'game'
$Cfg = Join-Path $GameDir 'fallout2.cfg'

# Dev profile: always windowed, no intro movies.
$lines = [System.IO.File]::ReadAllLines($Cfg)
$screenIdx = -1
for ($i = 0; $i -lt $lines.Length; $i++) {
    if ($lines[$i] -match '^\[screen\]') { $screenIdx = $i }
}
$want = @('[screen]', 'windowed=1')
if ($screenIdx -ge 0) {
    # replace windowed= line inside the [screen] section
    for ($i = $screenIdx + 1; $i -lt $lines.Length; $i++) {
        if ($lines[$i] -match '^\[') { break }
        if ($lines[$i] -match '^windowed=') { $lines[$i] = 'windowed=1' }
    }
} else {
    $lines = $lines + $want
}
[System.IO.File]::WriteAllLines($Cfg, $lines)

if (-not (Select-String -Path $Cfg -Pattern 'skip_opening_movies=1' -Quiet)) {
    Add-Content -Path $Cfg -Value "[ui]`nskip_opening_movies=1"
}

$toolsCommit = (git -C $Root rev-parse --short HEAD) 2>$null
if (-not $toolsCommit) { $toolsCommit = 'unknown' }

$exe = Join-Path $GameDir 'fallout2-ce.exe'
$start = if ($Slot -gt 0) { "--dev-load-game=$Slot" } else { '--dev-new-game' }
$args = @('--dev-tools', $start, "--dev-map=$Map", "--dev-elevation=$Elevation", "--dev-tools-commit=$toolsCommit")
Write-Output ("START: " + $exe + " " + ($args -join ' '))
Start-Process -FilePath $exe -ArgumentList $args -WorkingDirectory $GameDir
