# Task 169 probe: one default (entrance) still per desert/coast elevation.
param([string]$OutDir = "runs\_169_probe")
$ErrorActionPreference = 'Stop'
$dev = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$root = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $root 'build\scene-cache'

$targets = @(
    @{ map = 'desert2'; elev = 0 },
    @{ map = 'desert3'; elev = 0 },
    @{ map = 'desert4'; elev = 0 },
    @{ map = 'desert5'; elev = 0 },
    @{ map = 'desert5'; elev = 1 },
    @{ map = 'desert6'; elev = 0 },
    @{ map = 'desert6'; elev = 1 },
    @{ map = 'desert7'; elev = 0 },
    @{ map = 'desert8'; elev = 0 },
    @{ map = 'desert9'; elev = 0 },
    @{ map = 'desrt10'; elev = 0 },
    @{ map = 'desrt11'; elev = 0 },
    @{ map = 'desrt12'; elev = 0 },
    @{ map = 'desrt13'; elev = 0 },
    @{ map = 'desrt13'; elev = 1 },
    @{ map = 'desrt13'; elev = 2 },
    @{ map = 'coast1'; elev = 0 },
    @{ map = 'coast2'; elev = 0 },
    @{ map = 'coast2'; elev = 1 },
    @{ map = 'coast3'; elev = 0 },
    @{ map = 'coast4'; elev = 0 },
    @{ map = 'coast5'; elev = 0 },
    @{ map = 'coast5'; elev = 1 },
    @{ map = 'coast6'; elev = 0 },
    @{ map = 'coast7'; elev = 0 },
    @{ map = 'coast7'; elev = 1 },
    @{ map = 'coast8'; elev = 0 },
    @{ map = 'coast9'; elev = 0 },
    @{ map = 'coast10'; elev = 0 },
    @{ map = 'coast12'; elev = 0 }
)

foreach ($t in $targets) {
    $name = '{0}-e{1}' -f $t.map, $t.elev
    Write-Host $name
    & $dev -Map $t.map -Elevation $t.elev -Still -SceneCache $cache `
        -OutputDirectory (Join-Path (Join-Path $root $OutDir) $name) | Out-Null
}
Write-Host 'done'