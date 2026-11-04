# Task 124 evidence: the Den's carved cars from behind - the far sides take
# the art's own back views (CAR4b rot 3 at denbus1 17045, rot 4 at 20942)
# where the gates pass, and keep the near mirror where they do not (CAR3b's
# slots are placeholders, car3's opposite covers too little). The before
# cache is 123's own export, the after cache this branch's; the engine is
# the same binary, the change is exporter-only.
param(
    [string]$Before = 'build/cache-123b',
    [string]$After = 'build/cache-124'
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$devshot = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$cars = @(
    @{ name = 'car17045'; tile = 17045 },
    @{ name = 'car20942'; tile = 20942 }
)
foreach ($cache in @(@{ tag = 'before'; path = $Before }, @{ tag = 'after'; path = $After })) {
    foreach ($car in $cars) {
        foreach ($turn in @(0, 60, 120, 180, 240, 300)) {
            $dir = Join-Path $Root ("runs\_124_shots\{0}\{1}-turn{2}" -f $cache.tag, $car.name, $turn)
            New-Item -ItemType Directory -Path $dir -Force | Out-Null
            Write-Host ("{0} {1} turn {2}" -f $cache.tag, $car.name, $turn)
            & $devshot -Map 'denbus1' -Elevation 0 -Still -SceneCache $cache.path `
                -ViewTile $car.tile -Fly -FlyPitch 50 -FlyDistance 5 -FlyTurn $turn `
                -OutputDirectory $dir 2>&1 | Out-Null
        }
    }
}
Write-Host 'done'
