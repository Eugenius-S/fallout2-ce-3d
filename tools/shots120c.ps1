# Task 120 slice 2 captures: the Den's carved cars at four fly quarters.
# before = build\cache-120base (the card as the game had it), after = the
# carve's fly-only solid (build\cache-120carve).
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $Root 'runs\_120c_shots'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$devshot = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$targets = @(
    @{ name = 'car4b'; tile = 20942 },
    @{ name = 'car3b'; tile = 22956 }
)
foreach ($t in $targets) {
    foreach ($turn in @(0, 90, 180, 270)) {
        foreach ($state in @(@{ tag = 'before'; cache = 'build\cache-120base' }, @{ tag = 'after'; cache = 'build\cache-120carve' })) {
            $dir = Join-Path $out ("{0}-turn{1}-{2}" -f $t.name, $turn, $state.tag)
            New-Item -ItemType Directory -Path $dir -Force | Out-Null
            Write-Host ("{0} turn{1} {2}" -f $t.name, $turn, $state.tag)
            & $devshot -Map 'denbus1' -Elevation 0 -Still -SceneCache $state.cache `
                -ViewTile $t.tile -Fly -FlyPitch 35 -FlyDistance 9 -FlyTurn $turn `
                -OutputDirectory $dir | Out-Null
        }
    }
}
Write-Host 'done'
