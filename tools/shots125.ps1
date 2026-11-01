# Task 125 evidence: plain-fit boxes from behind - the far sides take the
# art's own back views where the gates pass (a game-visible cola machine
# denbus1 16510, the CAR4b rot 1 solid 16648, newrcs's shelf003 23105 and
# cola rot 3 18289) and keep the near mirror where they do not. The before
# cache is 124's own export, the after cache this branch's; the engine is
# the same binary, the change is exporter-only.
param(
    [string]$Before = 'build/cache-124',
    [string]$After = 'build/cache-125'
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$devshot = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$sites = @(
    @{ map = 'denbus1'; e = 0; name = 'cola16510'; tile = 16510; turns = @(0, 120, 180, 240) },
    @{ map = 'denbus1'; e = 0; name = 'car16648'; tile = 16648; turns = @(0, 120, 240) },
    @{ map = 'newrcs'; e = 0; name = 'shelf23105'; tile = 23105; turns = @(0, 120, 180) },
    @{ map = 'newrcs'; e = 0; name = 'cola18289'; tile = 18289; turns = @(120, 180) }
)
foreach ($cache in @(@{ tag = 'before'; path = $Before }, @{ tag = 'after'; path = $After })) {
    foreach ($site in $sites) {
        foreach ($turn in $site.turns) {
            $dir = Join-Path $Root ("runs\_125_shots\{0}\{1}-turn{2}" -f $cache.tag, $site.name, $turn)
            New-Item -ItemType Directory -Path $dir -Force | Out-Null
            Write-Host ("{0} {1} turn {2}" -f $cache.tag, $site.name, $turn)
            & $devshot -Map $site.map -Elevation $site.e -Still -SceneCache $cache.path `
                -ViewTile $site.tile -Fly -FlyPitch 50 -FlyDistance 5 -FlyTurn $turn `
                -OutputDirectory $dir 2>&1 | Out-Null
        }
    }
}
Write-Host 'done'
