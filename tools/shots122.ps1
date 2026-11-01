# Task 122 captures: a multi-side scenery thing (the Den's car) at two fly
# azimuths, the sprite-sides setting on and off. The off halves are what the
# game showed before (the made rotation); the on halves show the turned side.
param([string]$OutDir = "runs\_122_shots")
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $Root $OutDir
New-Item -ItemType Directory -Path $out -Force | Out-Null
$devshot = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$common = @{ Map = 'denbus1'; Elevation = 0; Still = $true; SceneCache = 'build\cache-122';
             ViewTile = 16648; Fly = $true; FlyPitch = 35; FlyDistance = 9 }
foreach ($turn in @(0, 90)) {
    foreach ($state in @(@{ tag = 'off'; sides = $false }, @{ tag = 'on'; sides = $true })) {
        $dir = Join-Path $out ("car-turn" + $turn + "-" + $state.tag)
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        Write-Host ("car-turn" + $turn + "-" + $state.tag)
        if ($state.sides) {
            & $devshot @common -FlyTurn $turn -OutputDirectory $dir | Out-Null
        } else {
            & $devshot @common -FlyTurn $turn -NoSpriteSides -OutputDirectory $dir | Out-Null
        }
    }
}
Write-Host 'done'
