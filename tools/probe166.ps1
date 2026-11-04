# Task 166 probe: one default (entrance) still per NCR / Vault 15 elevation.
param([string]$OutDir = "runs\_166_probe")
$ErrorActionPreference = 'Stop'
$dev = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$root = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $root 'build\scene-cache'

$targets = @(
    @{ map = 'v13ent'; elev = 0 }
    @{ map = 'vault13'; elev = 0 }
    @{ map = 'vault13'; elev = 1 }
    @{ map = 'vault13'; elev = 2 }
    @{ map = 'depolv1'; elev = 0 }
    @{ map = 'depolv1'; elev = 1 }
    @{ map = 'depolva'; elev = 0 }
    @{ map = 'depolva'; elev = 1 }
    @{ map = 'depolva'; elev = 2 }
    @{ map = 'depolvb'; elev = 0 }
)

foreach ($t in $targets) {
    $name = '{0}-e{1}' -f $t.map, $t.elev
    Write-Host $name
    & $dev -Map $t.map -Elevation $t.elev -Still -SceneCache $cache `
        -OutputDirectory (Join-Path (Join-Path $root $OutDir) $name) | Out-Null
}
Write-Host 'done'
