# Task 166 walk probe: walking keeps 3D on, one walk per area key map.
param([string]$OutDir = "runs\_166_walk")
$ErrorActionPreference = 'Stop'
$dev = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$root = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $root 'build\scene-cache'

$targets = @(
    @{ map = 'v13ent'; elev = 0; tile = 25943 },
    @{ map = 'vault13'; elev = 0; tile = 17095 },
    @{ map = 'vault13'; elev = 2; tile = 16694 },
    @{ map = 'depolv1'; elev = 0; tile = 27705 },
    @{ map = 'depolva'; elev = 1; tile = 26504 },
    @{ map = 'depolvb'; elev = 0; tile = 21910 }
)

foreach ($t in $targets) {
    $name = '{0}-e{1}' -f $t.map, $t.elev
    Write-Host $name
    & $dev -Map $t.map -Elevation $t.elev -ViewTile $t.tile -WalkProbe -HeroTile $t.tile `
        -SceneCache $cache -OutputDirectory (Join-Path (Join-Path $root $OutDir) $name) | Out-Null
}
Write-Host 'done'
