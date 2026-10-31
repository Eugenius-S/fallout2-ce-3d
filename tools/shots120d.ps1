# Task 120 carve close-ups: the cars at a steep orbit where the solid's
# volume reads against the flat card.
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
            $dir = Join-Path $out ("{0}-close{1}-{2}" -f $t.name, $turn, $state.tag)
            New-Item -ItemType Directory -Path $dir -Force | Out-Null
            Write-Host ("{0} close{1} {2}" -f $t.name, $turn, $state.tag)
            & $devshot -Map 'denbus1' -Elevation 0 -Still -SceneCache $state.cache `
                -ViewTile $t.tile -Fly -FlyPitch 50 -FlyDistance 5 -FlyTurn $turn `
                -OutputDirectory $dir | Out-Null
        }
    }
}
Write-Host 'done'
