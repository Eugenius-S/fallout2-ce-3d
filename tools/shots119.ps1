# Acceptance shots for task 119: the stands at the game's angle and in flight,
# before (the baseline cache) and after (the rules cache). Four captures a
# target: <name>-game-<before|after>, <name>-fly-<before|after>.
param([string]$OutDir = "runs\_119_shots_fix")
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$before = Join-Path $Root 'build\scene-cache'
$after = Join-Path $Root 'build\cache-119-fix'
$out = Join-Path $Root $OutDir
New-Item -ItemType Directory -Path $out -Force | Out-Null

# name, map, elev, viewTile (the game's angle), flyView (the orbit centre), heroTile (0 = none)
$targets = @(
    @{ name = 'stove-bathhouse'; map = 'klamall';  elev = 0; view = 24536; flyView = 24138; hero = 0 },
    @{ name = 'stove-downtown';  map = 'kladwtwn'; elev = 0; view = 21335; flyView = 20937; hero = 0 },
    @{ name = 'stove-warehouse'; map = 'kladwtwn'; elev = 0; view = 22926; flyView = 22528; hero = 0 },
    @{ name = 'stove-east';      map = 'kladwtwn'; elev = 0; view = 26504; flyView = 26106; hero = 0 },
    @{ name = 'lockers-mall';    map = 'klamall';  elev = 0; view = 15683; flyView = 15683; hero = 15683 },
    @{ name = 'locker-ratcave';  map = 'klaratcv'; elev = 1; view = 27697; flyView = 27697; hero = 27697 }
)

foreach ($t in $targets) {
    foreach ($side in @(@{ tag = 'before'; cache = $before }, @{ tag = 'after'; cache = $after })) {
        $devshot = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
        $common = @{ Map = $t.map; Elevation = $t.elev; Still = $true; SceneCache = $side.cache }
        if ($t.hero -gt 0) { $common.HeroTile = $t.hero }
        $dir = Join-Path $out ($t.name + '-game-' + $side.tag)
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        Write-Host ($t.name + '-game-' + $side.tag)
        & $devshot @common -ViewTile $t.view -OutputDirectory $dir | Out-Null
        $dir = Join-Path $out ($t.name + '-fly-' + $side.tag)
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        Write-Host ($t.name + '-fly-' + $side.tag)
        & $devshot @common -ViewTile $t.flyView -Fly -FlyTurn 45 -FlyPitch 25 -FlyDistance 6 `
            -OutputDirectory $dir | Out-Null
    }
}
Write-Host 'done'
