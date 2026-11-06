# Task 167 sweep: military base spots at morning, noon and evening,
# plus the game's own 2D (Classic) for the dark views.
param([string]$OutDir = "runs\_167_hours")
$ErrorActionPreference = 'Stop'
$dev = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$root = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $root 'build\scene-cache'

$spots = @(
    @{ map = 'mbclose'; elev = 0; tile = 23939 },
    @{ map = 'mbase12'; elev = 0; tile = 16089 },
    @{ map = 'mbase12'; elev = 1; tile = 16089 },
    @{ map = 'mbase34'; elev = 0; tile = 14141 },
    @{ map = 'mbase34'; elev = 1; tile = 15131 },
    @{ map = 'mbase34'; elev = 2; tile = 14141 }
)

foreach ($s in $spots) {
    foreach ($h in @(8, 12, 18)) {
        $name = '{0}-e{1}-h{2}' -f $s.map, $s.elev, $h
        Write-Host $name
        & $dev -Map $s.map -Elevation $s.elev -Still -ViewTile $s.tile `
            -Fly -FlyTurn 0 -FlyPitch 35 -FlyDistance 8 -Hour $h -SceneCache $cache `
            -OutputDirectory (Join-Path (Join-Path $root $OutDir) $name) | Out-Null
    }
}

# The dark views against the game's own 2D picture.
foreach ($s in @(
    @{ map = 'mbase12'; elev = 0; tile = 16089 },
    @{ map = 'mbase34'; elev = 0; tile = 14141 }
)) {
    $name = 'classic-{0}-e{1}' -f $s.map, $s.elev
    Write-Host $name
    & $dev -Map $s.map -Elevation $s.elev -Still -ViewTile $s.tile -Classic `
        -SceneCache $cache -OutputDirectory (Join-Path (Join-Path $root $OutDir) $name) | Out-Null
}
Write-Host 'done'
