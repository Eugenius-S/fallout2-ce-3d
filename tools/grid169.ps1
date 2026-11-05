# Task 169 grid probe: camera coverage on three representative encounter maps.
param([string]$OutDir = "runs\_169_grid")
$ErrorActionPreference = 'Stop'
$dev = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$root = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $root 'build\scene-cache'

$spots = @(
    @{ map = 'coast1'; elev = 0; tile = 20100 },
    @{ map = 'desert5'; elev = 0; tile = 21099 },
    @{ map = 'coast10'; elev = 0; tile = 19716 }
)

foreach ($s in $spots) {
    foreach ($d in @(-6000, -4000, -2000, 2000, 4000, 6000)) {
        $t = $s.tile + $d
        if ($t -lt 0 -or $t -gt 39999) { continue }
        $name = '{0}-e{1}-t{2}' -f $s.map, $s.elev, $t
        Write-Host $name
        & $dev -Map $s.map -Elevation $s.elev -Still -ViewTile $t -SceneCache $cache `
            -OutputDirectory (Join-Path (Join-Path $root $OutDir) $name) | Out-Null
    }
}
Write-Host 'done'