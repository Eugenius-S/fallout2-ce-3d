# Task 169: the game's own 2D for the night-dark encounter spots.
param([string]$OutDir = "runs\_169_hours")
$ErrorActionPreference = 'Stop'
$dev = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$root = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $root 'build\scene-cache'

foreach ($s in @(
    @{ map = 'coast2'; elev = 0; tile = 20100 },
    @{ map = 'coast7'; elev = 0; tile = 20101 },
    @{ map = 'desrt13'; elev = 0; tile = 20100 }
)) {
    $name = 'classic-{0}-e{1}' -f $s.map, $s.elev
    Write-Host $name
    & $dev -Map $s.map -Elevation $s.elev -Still -ViewTile $s.tile -Classic `
        -SceneCache $cache -OutputDirectory (Join-Path (Join-Path $root $OutDir) $name) | Out-Null
}
Write-Host 'done'
