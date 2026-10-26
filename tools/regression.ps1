# Export the spots' maps with this checkout's exporter, capture fixed views
# with the existing build; compare or explicitly update.
param(
    [switch]$Update,
    [string]$Spot
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Checker = Join-Path $PSScriptRoot 'regression\check.py'
$manifest = Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'regression\spots.json') | ConvertFrom-Json
$screen = $manifest.screen
$select = @()
if ($Spot) { $select = @('--spot', $Spot) }
$list = & py -3 $Checker --list @select
if ($LASTEXITCODE -ne 0) { throw 'Invalid regression spot selection.' }
$spots = $list | ConvertFrom-Json
# The scenes come from this checkout's exporter, never from whatever cache the
# owner's game last got: build/scene-cache is what dev-snapshot reads.
$gameDir = Join-Path $Root 'game'
if (-not (Test-Path -LiteralPath $gameDir)) {
    $main = (git -C $Root worktree list --porcelain | Select-Object -First 1) -replace '^worktree ', ''
    $gameDir = Join-Path $main 'game'
}
$maps = ($spots | ForEach-Object { $_.map } | Sort-Object -Unique) -join ','
Write-Host "Exporting $maps to build\scene-cache"
& py -3 (Join-Path $PSScriptRoot 'scene_export\export.py') --game-root $gameDir --maps $maps `
    --cache-root (Join-Path $Root 'build\scene-cache') | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'The scene export failed.' }
$run = Join-Path $Root ('runs\regression\' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '-' + $PID)
New-Item -ItemType Directory -Path $run -Force | Out-Null
Write-Host "REGRESSION: $run"
foreach ($entry in $spots) {
    $directory = Join-Path $run $entry.name
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
    Write-Host "Capturing $($entry.name) ($($entry.map) e$($entry.elevation), tile $($entry.tile))"
    # A test game now and then shows no snapshot within the timeout: one more
    # try, the first error kept as capture-error-1.txt.
    for ($attempt = 1; $attempt -le 2; $attempt++) {
        $failure = Join-Path $directory 'capture-error.txt'
        if (Test-Path -LiteralPath $failure) { Move-Item -Force -LiteralPath $failure (Join-Path $directory 'capture-error-1.txt') }
        try {
            & (Join-Path $PSScriptRoot 'dev-snapshot.ps1') -Map $entry.map -Elevation $entry.elevation -ViewTile $entry.tile -Still -CaptureConsole `
                -ScreenWidth $screen.width -ScreenHeight $screen.height -Scale $screen.scale -OutputDirectory $directory |
                Set-Content -LiteralPath (Join-Path $directory 'capture.log') -Encoding UTF8
            break
        } catch {
            $_ | Out-String | Set-Content -LiteralPath $failure -Encoding UTF8
            Write-Host "Capture failed: $_"
            if (Get-ChildItem -LiteralPath $directory -Filter 'devshot-*.json') { break }
        }
    }
}
$mode = @()
if ($Update) { $mode = @('--update') }
& py -3 $Checker --run $run @select @mode
exit $LASTEXITCODE
