# Capture fixed views with the existing build; compare or explicitly update.
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
$run = Join-Path $Root ('runs\regression\' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '-' + $PID)
New-Item -ItemType Directory -Path $run -Force | Out-Null
Write-Host "REGRESSION: $run"
foreach ($entry in $spots) {
    $directory = Join-Path $run $entry.name
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
    Write-Host "Capturing $($entry.name) ($($entry.map) e$($entry.elevation), tile $($entry.tile))"
    try {
        & (Join-Path $PSScriptRoot 'dev-snapshot.ps1') -Map $entry.map -Elevation $entry.elevation -ViewTile $entry.tile -Still `
            -ScreenWidth $screen.width -ScreenHeight $screen.height -Scale $screen.scale -OutputDirectory $directory |
            Set-Content -LiteralPath (Join-Path $directory 'capture.log') -Encoding UTF8
    } catch {
        $_ | Out-String | Set-Content -LiteralPath (Join-Path $directory 'capture-error.txt') -Encoding UTF8
        Write-Host "Capture failed: $_"
    }
}
$mode = @()
if ($Update) { $mode = @('--update') }
& py -3 $Checker --run $run @select @mode
exit $LASTEXITCODE
