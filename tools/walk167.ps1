# Task 167 walk probe: walking keeps 3D on, one walk per military base map.
param([string]$OutDir = "runs\_167_walk")
$ErrorActionPreference = 'Stop'
$dev = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$root = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $root 'build\scene-cache'

$targets = @(
    @{ map = 'mbclose'; elev = 0; tile = 23939 },
    @{ map = 'mbase12'; elev = 0; tile = 16089 },
    @{ map = 'mbase12'; elev = 1; tile = 16089 },
    @{ map = 'mbase34'; elev = 1; tile = 15131 }
)

foreach ($t in $targets) {
    $name = '{0}-e{1}' -f $t.map, $t.elev
    Write-Host $name
    & $dev -Map $t.map -Elevation $t.elev -ViewTile $t.tile -WalkProbe -HeroTile $t.tile `
        -SceneCache $cache -OutputDirectory (Join-Path (Join-Path $root $OutDir) $name) | Out-Null
}
Write-Host 'done'
