# Background test run: starts the freshly built game in the 3D scene, off
# screen and without taking focus, takes one metadata snapshot and closes only
# that process. The build runs as game/fallout2-ce-test.exe (the game reads
# fallout2.cfg next to its exe), so a game the user has open is left alone, and
# so is game/fallout2-ce.exe (dev-launch.ps1 installs new builds).
#
# Prints the snapshot JSON; the image lies next to it in game/.
param(
    [string]$Map = "kladwtwn",
    [int]$Elevation = 0,
    [int]$TimeoutSeconds = 90
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$GameDir = Join-Path $Root 'game'
$built = Join-Path $Root 'build\fallout2-ce.exe'
if (-not (Test-Path $built)) { throw "No build at $built; run tools/configure-build.ps1 first." }
$exe = Join-Path $GameDir 'fallout2-ce-test.exe'
Copy-Item $built $exe -Force

$started = Get-Date
$env:F2CE_DEV_BACKGROUND = '1'
try {
    $process = Start-Process -FilePath $exe -WorkingDirectory $GameDir -PassThru -ArgumentList @(
        '--dev-tools', '--dev-new-game', "--dev-map=$Map", "--dev-elevation=$Elevation",
        '--dev-hero-preview', '--dev-snapshot-once')
} finally {
    Remove-Item Env:F2CE_DEV_BACKGROUND
}

$snapshot = $null
$deadline = $started.AddSeconds($TimeoutSeconds)
while ((Get-Date) -lt $deadline -and -not $process.HasExited) {
    $snapshot = Get-ChildItem $GameDir -Filter 'devshot-*.json' |
        Where-Object LastWriteTime -gt $started | Select-Object -First 1
    if ($snapshot) { break }
    Start-Sleep -Milliseconds 500
}
Start-Sleep -Milliseconds 500
if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
if (-not $snapshot) { throw "No snapshot within $TimeoutSeconds s." }
Write-Output ("SNAPSHOT: " + $snapshot.FullName)
Get-Content $snapshot.FullName
