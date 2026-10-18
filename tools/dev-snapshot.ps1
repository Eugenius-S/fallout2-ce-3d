# Background test run: starts the freshly built game in the 3D scene, off
# screen and without taking focus, takes one metadata snapshot and closes only
# that process. The build runs as game/fallout2-ce-test.exe (the game reads
# fallout2.cfg next to its exe), so a game the user has open is left alone, and
# so is game/fallout2-ce.exe (dev-launch.ps1 installs new builds).
#
# -Probe: the mouse is put on the scene's critters and what the game finds
# under it goes to game/devprobe.log; two snapshots show the hex cursor and
# the hover icon over a critter, then a left click there is logged until the
# game leaves the scene (a dialog).
#
# Prints the snapshot JSON; the image lies next to it in game/.
param(
    [string]$Map = "kladwtwn",
    [int]$Elevation = 0,
    [int]$TimeoutSeconds = 90,
    [switch]$Probe
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$GameDir = Join-Path $Root 'game'
$built = Join-Path $Root 'build\fallout2-ce.exe'
if (-not (Test-Path $built)) { throw "No build at $built; run tools/configure-build.ps1 first." }
$exe = Join-Path $GameDir 'fallout2-ce-test.exe'
Copy-Item $built $exe -Force

$shots = 1
$arguments = @('--dev-tools', '--dev-new-game', "--dev-map=$Map", "--dev-elevation=$Elevation", '--dev-hero-preview')
if ($Probe) {
    $shots = 2
    $arguments += '--dev-pick-probe'
    Remove-Item (Join-Path $GameDir 'devprobe.log') -ErrorAction SilentlyContinue
} else {
    $arguments += '--dev-snapshot-once'
}

$started = Get-Date
$env:F2CE_DEV_BACKGROUND = '1'
try {
    $process = Start-Process -FilePath $exe -WorkingDirectory $GameDir -PassThru -ArgumentList $arguments
} finally {
    Remove-Item Env:F2CE_DEV_BACKGROUND
}

$snapshots = @()
$deadline = $started.AddSeconds($TimeoutSeconds)
while ((Get-Date) -lt $deadline -and -not $process.HasExited) {
    $snapshots = @(Get-ChildItem $GameDir -Filter 'devshot-*.json' | Where-Object LastWriteTime -gt $started)
    $log = Join-Path $GameDir 'devprobe.log'
    $probed = -not $Probe -or ((Test-Path $log) -and (Select-String -Path $log -Pattern 'probe done' -Quiet))
    if ($snapshots.Count -ge $shots -and $probed) { break }
    Start-Sleep -Milliseconds 500
}
Start-Sleep -Milliseconds 500
if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
if ($snapshots.Count -lt $shots) { throw "No snapshot within $TimeoutSeconds s." }
foreach ($snapshot in $snapshots) {
    Write-Output ("SNAPSHOT: " + $snapshot.FullName)
    Get-Content $snapshot.FullName
}
if ($Probe) {
    Write-Output "PROBE:"
    Get-Content (Join-Path $GameDir 'devprobe.log')
}
