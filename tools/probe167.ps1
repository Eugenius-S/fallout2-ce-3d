# Task 167 probe: one default (entrance) still per military base elevation.
param([string]$OutDir = "runs\_167_probe")
$ErrorActionPreference = 'Stop'
$dev = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$root = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $root 'build\scene-cache'

$targets = @(
    @{ map = 'mbclose'; elev = 0 },
    @{ map = 'mbase12'; elev = 0 },
    @{ map = 'mbase12'; elev = 1 },
    @{ map = 'mbase34'; elev = 0 },
    @{ map = 'mbase34'; elev = 1 },
    @{ map = 'mbase34'; elev = 2 }
)

foreach ($t in $targets) {
    $name = '{0}-e{1}' -f $t.map, $t.elev
    Write-Host $name
    & $dev -Map $t.map -Elevation $t.elev -Still -SceneCache $cache `
        -OutputDirectory (Join-Path (Join-Path $root $OutDir) $name) | Out-Null
}
Write-Host 'done'
