# Task 169 walk probe: walking keeps 3D on.
param([string]$OutDir = "runs\_169_walk")
$ErrorActionPreference = 'Stop'
$dev = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$root = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $root 'build\scene-cache'

$targets = @(
    @{ map = 'coast1'; elev = 0; tile = 20100 },
    @{ map = 'coast10'; elev = 0; tile = 19716 },
    @{ map = 'coast12'; elev = 0; tile = 23313 },
    @{ map = 'coast2'; elev = 0; tile = 20100 },
    @{ map = 'coast3'; elev = 0; tile = 18488 },
    @{ map = 'coast4'; elev = 0; tile = 18488 }
)

foreach ($t in $targets) {
    $name = '{0}-e{1}' -f $t.map, $t.elev
    Write-Host $name
    & $dev -Map $t.map -Elevation $t.elev -ViewTile $t.tile -WalkProbe -HeroTile $t.tile `
        -SceneCache $cache -OutputDirectory (Join-Path (Join-Path $root $OutDir) $name) | Out-Null
}
Write-Host 'done'