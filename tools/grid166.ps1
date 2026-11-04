# Task 166 grid probe: sweep view tiles around the busy rooms of the biggest
# maps, so the tour and regression spots land on real content.
param([string]$OutDir = "runs\_166_grid")
$ErrorActionPreference = 'Stop'
$dev = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$root = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $root 'build\scene-cache'

$spots = @(
    @{ map = 'v13ent'; elev = 0; tile = 25943 },
    @{ map = 'vault13'; elev = 0; tile = 17095 },
    @{ map = 'vault13'; elev = 2; tile = 16694 },
    @{ map = 'depolv1'; elev = 0; tile = 27705 },
    @{ map = 'depolva'; elev = 1; tile = 26504 },
    @{ map = 'depolvb'; elev = 0; tile = 21910 }
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
