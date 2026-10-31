# Regression capture against an experiment scene cache, then check against references.
param(
    [string]$SceneCache = "build\cache-119-fix",
    [string]$RunName = "119-fix"
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Checker = Join-Path $PSScriptRoot 'regression\check.py'
$manifest = Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'regression\spots.json') | ConvertFrom-Json
$screen = $manifest.screen
$spots = & py -3 $Checker --list | ConvertFrom-Json
$cache = Join-Path $Root $SceneCache
if (-not (Test-Path -LiteralPath $cache)) { throw "No scene cache at $cache" }
$run = Join-Path $Root ("runs\regression\" + (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '-' + $RunName)
New-Item -ItemType Directory -Path $run -Force | Out-Null
Write-Host "REGRESSION: $run"
foreach ($entry in $spots) {
    $directory = Join-Path $run $entry.name
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
    Write-Host "Capturing $($entry.name) ($($entry.map) e$($entry.elevation), tile $($entry.tile))"
    for ($attempt = 1; $attempt -le 2; $attempt++) {
        $failure = Join-Path $directory 'capture-error.txt'
        if (Test-Path -LiteralPath $failure) { Move-Item -Force -LiteralPath $failure (Join-Path $directory 'capture-error-1.txt') }
        try {
            & (Join-Path $PSScriptRoot 'dev-snapshot.ps1') -Map $entry.map -Elevation $entry.elevation -ViewTile $entry.tile -Still -CaptureConsole `
                -SceneCache $cache `
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
& py -3 $Checker --run $run
exit $LASTEXITCODE
