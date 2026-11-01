# Acceptance shots for task 120: the drum wall trim and the fly-only solids'
# scene light. Pairs are 119's reviewed state (cache-119-fix) | this task's
# (cache-120), so a pair shows exactly what changed since the owner's pass.
# Four captures a target: <name>-game-<before|after>, <name>-fly-<before|after>.
param([string]$OutDir = "runs\_120_shots")
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$before = Join-Path $Root 'build\cache-119-fix'
$after = Join-Path $Root 'build\cache-120'
$out = Join-Path $Root $OutDir
New-Item -ItemType Directory -Path $out -Force | Out-Null

# name, map, elev, viewTile (the game's angle), turn (camera quarter turns),
# flyView (the orbit centre), flyTurn/flyPitch/flyDistance, heroTile (0 = none)
$targets = @(
    @{ name = 'stove-bathhouse'; map = 'klamall';  elev = 0; view = 24138; turn = 2;
       flyView = 24138; flyTurn = 135; flyPitch = 35; flyDist = 4; hero = 0 },
    @{ name = 'stove-downtown';  map = 'kladwtwn'; elev = 0; view = 20937; turn = 0;
       flyView = 20937; flyTurn = 225; flyPitch = 45; flyDist = 7; hero = 0 },
    @{ name = 'stove-warehouse'; map = 'kladwtwn'; elev = 0; view = 22528; turn = 0;
       flyView = 22528; flyTurn = 225; flyPitch = 45; flyDist = 7; hero = 0 },
    @{ name = 'stove-east';      map = 'kladwtwn'; elev = 0; view = 26106; turn = 0;
       flyView = 26106; flyTurn = 225; flyPitch = 45; flyDist = 7; hero = 0 },
    @{ name = 'lockers-mall';    map = 'klamall';  elev = 0; view = 15683; turn = 0;
       flyView = 15683; flyTurn = 45; flyPitch = 45; flyDist = 6; hero = 15683 },
    @{ name = 'lockers-mall-back'; map = 'klamall'; elev = 0; view = 15683; turn = 0;
       flyView = 15683; flyTurn = 225; flyPitch = 45; flyDist = 6; hero = 15683 },
    @{ name = 'locker-ratcave';  map = 'klaratcv'; elev = 1; view = 27697; turn = 0;
       flyView = 27697; flyTurn = 45; flyPitch = 45; flyDist = 6; hero = 27697 },
    @{ name = 'locker-ratcave-back'; map = 'klaratcv'; elev = 1; view = 27697; turn = 0;
       flyView = 27697; flyTurn = 225; flyPitch = 45; flyDist = 6; hero = 27697 }
)

foreach ($t in $targets) {
    foreach ($side in @(@{ tag = 'before'; cache = $before }, @{ tag = 'after'; cache = $after })) {
        $devshot = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
        $common = @{ Map = $t.map; Elevation = $t.elev; Still = $true; SceneCache = $side.cache }
        if ($t.hero -gt 0) { $common.HeroTile = $t.hero }
        $dir = Join-Path $out ($t.name + '-game-' + $side.tag)
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        Write-Host ($t.name + '-game-' + $side.tag)
        & $devshot @common -ViewTile $t.view -Turn $t.turn -OutputDirectory $dir | Out-Null
        $dir = Join-Path $out ($t.name + '-fly-' + $side.tag)
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        Write-Host ($t.name + '-fly-' + $side.tag)
        & $devshot @common -ViewTile $t.flyView -Fly -FlyTurn $t.flyTurn -FlyPitch $t.flyPitch `
            -FlyDistance $t.flyDist -OutputDirectory $dir | Out-Null
    }
}
Write-Host 'done'
