# Task 169: the desert1 spot the cache-hit export log line missed.
param([string]$OutDir = "runs\_169_hours")
$ErrorActionPreference = 'Stop'
$dev = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$root = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $root 'build\scene-cache'

foreach ($h in @(8, 12, 18)) {
    $name = 'desert1-e0-h{0}' -f $h
    Write-Host $name
    & $dev -Map desert1 -Elevation 0 -Still -ViewTile 17288 `
        -Fly -FlyTurn 0 -FlyPitch 35 -FlyDistance 8 -Hour $h -SceneCache $cache `
        -OutputDirectory (Join-Path (Join-Path $root $OutDir) $name) | Out-Null
}
Write-Host "desert1-e0"
& $dev -Map desert1 -Elevation 0 -ViewTile 17288 -WalkProbe -HeroTile 17288 `
    -SceneCache $cache -OutputDirectory (Join-Path (Join-Path $root 'runs\_169_walk') 'desert1-e0') | Out-Null
Write-Host 'done'
