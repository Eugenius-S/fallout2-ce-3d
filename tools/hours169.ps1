# Task 169 sweep: desert/coast spots at morning, noon and evening.
param([string]$OutDir = "runs\_169_hours")
$ErrorActionPreference = 'Stop'
$dev = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$root = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $root 'build\scene-cache'

$spots = @(
    @{ map = 'coast1'; elev = 0; tile = 20100 },
    @{ map = 'coast10'; elev = 0; tile = 19716 },
    @{ map = 'coast12'; elev = 0; tile = 23313 },
    @{ map = 'coast2'; elev = 0; tile = 20100 },
    @{ map = 'coast2'; elev = 1; tile = 100 },
    @{ map = 'coast3'; elev = 0; tile = 18488 },
    @{ map = 'coast4'; elev = 0; tile = 18488 },
    @{ map = 'coast5'; elev = 0; tile = 21103 },
    @{ map = 'coast5'; elev = 1; tile = 21103 },
    @{ map = 'coast6'; elev = 0; tile = 20100 },
    @{ map = 'coast7'; elev = 0; tile = 18311 },
    @{ map = 'coast7'; elev = 1; tile = 160 },
    @{ map = 'coast8'; elev = 0; tile = 22912 },
    @{ map = 'coast9'; elev = 0; tile = 20100 },
    @{ map = 'desert2'; elev = 0; tile = 20100 },
    @{ map = 'desert3'; elev = 0; tile = 20100 },
    @{ map = 'desert4'; elev = 0; tile = 19312 },
    @{ map = 'desert5'; elev = 0; tile = 21099 },
    @{ map = 'desert5'; elev = 1; tile = 508 },
    @{ map = 'desert6'; elev = 0; tile = 20100 },
    @{ map = 'desert6'; elev = 1; tile = 36 },
    @{ map = 'desert7'; elev = 0; tile = 20101 },
    @{ map = 'desert8'; elev = 0; tile = 20100 },
    @{ map = 'desert9'; elev = 0; tile = 21894 },
    @{ map = 'desrt10'; elev = 0; tile = 21894 },
    @{ map = 'desrt11'; elev = 0; tile = 17685 },
    @{ map = 'desrt12'; elev = 0; tile = 20100 },
    @{ map = 'desrt13'; elev = 0; tile = 20100 },
    @{ map = 'desrt13'; elev = 1; tile = 20100 },
    @{ map = 'desrt13'; elev = 2; tile = 20100 }
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
Write-Host 'done'