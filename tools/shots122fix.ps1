# Task 122 fix evidence: TIRE004 (denbus1 21943) at every fly quarter - the
# gate passes it today while four of its six slots are UNUSED ART placeholder
# cards, so the turned card shows the green placeholder. One cache (built on
# this branch) serves both the before and the after shots: the fix is
# engine-only and does not touch the export.
param(
    [string]$Cache = 'build/cache-122',
    [string]$Tag = 'before'
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $Root ("runs\_122fix_shots\{0}" -f $Tag)
New-Item -ItemType Directory -Path $out -Force | Out-Null
$devshot = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
foreach ($turn in @(0, 60, 120, 180, 240, 300)) {
    $dir = Join-Path $out ("tire{0}" -f $turn)
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
    Write-Host ("tire turn {0}" -f $turn)
    & $devshot -Map 'denbus1' -Elevation 0 -Still -SceneCache $Cache `
        -ViewTile 21943 -Fly -FlyPitch 50 -FlyDistance 5 -FlyTurn $turn `
        -OutputDirectory $dir 2>&1 | Tee-Object -FilePath ("runs\_122fix_shots\{0}-turn{1}.log" -f $Tag, $turn) | Out-Null
}
Write-Host 'done'
