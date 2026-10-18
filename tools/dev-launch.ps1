# Dev-mode launcher for the sprite-lighting mod (spec edition 2, P1).
#
# Prepares the working copy config (windowed, no intro) and starts
# fallout2-ce.exe with the dev tools. The default target is Klamath
# Downtown (kladwtwn), elevation 0. The save slot 1 in game/data/SAVEGAME
# is the fixed dev profile (a fully created character); the dev panel
# (Alt+D) travels between maps, F9 takes a metadata snapshot.
param(
    [string]$Map = "kladwtwn",
    [int]$Elevation = 0,
    [int]$Slot = 1
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

$exe = Join-Path $GameDir 'fallout2-ce.exe'
$args = @('--dev-tools', "--dev-load-game=$Slot", "--dev-map=$Map", "--dev-elevation=$Elevation")
Write-Output ("START: " + $exe + " " + ($args -join ' '))
Start-Process -FilePath $exe -ArgumentList $args -WorkingDirectory $GameDir
