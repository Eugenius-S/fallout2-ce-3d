# Task 169: the game's own 2D for the dark second elevations.
param([string]$OutDir = "runs\_169_classic_e1")
$ErrorActionPreference = 'Stop'
$dev = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$root = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $root 'build\scene-cache'

foreach ($s in @(
    @{ map = 'coast5'; elev = 1; tile = 20100 },
    @{ map = 'coast7'; elev = 1; tile = 20100 },
    @{ map = 'desert6'; elev = 1; tile = 20100 },
    @{ map = 'desrt13'; elev = 1; tile = 20100 },
    @{ map = 'desrt13'; elev = 2; tile = 20100 }
)) {
    $name = '{0}-e{1}' -f $s.map, $s.elev
    Write-Host $name
    & $dev -Map $s.map -Elevation $s.elev -Still -ViewTile $s.tile -Classic `
        -SceneCache $cache -OutputDirectory (Join-Path (Join-Path $root $OutDir) $name) | Out-Null
}
Write-Host 'done'
