# Task 166 sweep: Vault 13 / Sierra Depot spots at morning, noon and evening,
# plus the game's own 2D (Classic) for the dark views.
param([string]$OutDir = "runs\_166_hours")
$ErrorActionPreference = 'Stop'
$dev = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$root = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $root 'build\scene-cache'

$spots = @(
    @{ map = 'v13ent'; elev = 0; tile = 25943 },
    @{ map = 'vault13'; elev = 0; tile = 17095 },
    @{ map = 'vault13'; elev = 1; tile = 17095 },
    @{ map = 'vault13'; elev = 2; tile = 16694 },
    @{ map = 'depolv1'; elev = 0; tile = 27705 },
    @{ map = 'depolv1'; elev = 1; tile = 21321 },
    @{ map = 'depolva'; elev = 0; tile = 26505 },
    @{ map = 'depolva'; elev = 1; tile = 26504 },
    @{ map = 'depolva'; elev = 2; tile = 26104 },
    @{ map = 'depolvb'; elev = 0; tile = 21910 }
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
    @{ map = 'depolv1'; elev = 1; tile = 21321 },
    @{ map = 'depolva'; elev = 2; tile = 26104 },
    @{ map = 'v13ent'; elev = 0; tile = 25943 }
)) {
    $name = 'classic-{0}-e{1}' -f $s.map, $s.elev
    Write-Host $name
    & $dev -Map $s.map -Elevation $s.elev -Still -ViewTile $s.tile -Classic `
        -SceneCache $cache -OutputDirectory (Join-Path (Join-Path $root $OutDir) $name) | Out-Null
}
Write-Host 'done'
