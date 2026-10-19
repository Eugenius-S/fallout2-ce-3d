# Background test run: starts the freshly built game in the 3D scene, off
# screen and without taking focus, takes one metadata snapshot and closes only
# that process. The build runs as game/fallout2-ce-test.exe (the game reads
# fallout2.cfg next to its exe), so a game the user has open is left alone, and
# so is game/fallout2-ce.exe (dev-launch.ps1 installs new builds).
#
# -Probe: the mouse is put on the scene's critters and what the game finds
# under it goes to game/devprobe.log; two snapshots show the hex cursor and
# the hover icon over a critter, then a left click there is logged until the
# game leaves the scene (a dialog).
#
# -Tour "klatrap:0,kladwtwn:0": after the snapshot on -Map, the same process
# travels to each listed map elevation and takes one snapshot there.
#
# Prints the snapshot JSON; the image lies next to it in game/.
param(
    [string]$Map = "kladwtwn",
    [int]$Elevation = 0,
    [int]$TimeoutSeconds = 90,
    [switch]$Probe,
    [string]$Tour,
    [ValidateRange(0, 39999)]
    [int]$ViewTile,
    [switch]$Still,
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$GameDir = Join-Path $Root 'game'
if (-not (Test-Path -LiteralPath $GameDir)) {
    # An agent's worktree has no game: use the main checkout's, with this build.
    $main = (git -C $Root worktree list --porcelain | Select-Object -First 1) -replace '^worktree ', ''
    $GameDir = Join-Path $main 'game'
}
$built = Join-Path $Root 'build\fallout2-ce.exe'
if (-not (Test-Path $built)) { throw "No build at $built; run tools/configure-build.ps1 first." }
$exe = Join-Path $GameDir 'fallout2-ce-test.exe'
if (-not (Select-String -LiteralPath (Join-Path $GameDir 'fallout2.cfg') -Pattern '^windowed\s*=\s*1\s*$' -Quiet)) {
    throw 'Background snapshots require windowed=1 in game/fallout2.cfg.'
}
if (-not (Test-Path -LiteralPath $exe) -or
    (Get-FileHash -LiteralPath $built).Hash -ne (Get-FileHash -LiteralPath $exe).Hash) {
    Copy-Item $built $exe -Force
}

$shots = 1
$runId = [guid]::NewGuid().ToString('N')
$arguments = @('--dev-tools', '--dev-new-game', "--dev-map=$Map", "--dev-elevation=$Elevation", '--dev-hero-preview', "--dev-snapshot-run=$runId")
if ($PSBoundParameters.ContainsKey('ViewTile')) { $arguments += "--dev-view-tile=$ViewTile" }
if ($Still) {
    if ($Probe) { throw '-Still cannot be combined with the interactive -Probe.' }
    $arguments += '--dev-still-snapshot'
}
if ($Probe) {
    if ($Tour) { throw '-Tour cannot be combined with -Probe.' }
    $shots = 2
    $arguments += '--dev-pick-probe'
    Remove-Item (Join-Path $GameDir 'devprobe.log') -ErrorAction SilentlyContinue
} elseif ($Tour) {
    $shots = 1 + @($Tour.Split(',') | Where-Object { $_.Trim() }).Count
    $arguments += "--dev-map-tour=$Tour"
} else {
    $arguments += '--dev-snapshot-once'
}

$started = Get-Date
$previousBackground = $env:F2CE_DEV_BACKGROUND
$env:F2CE_DEV_BACKGROUND = '1'
$process = $null
$snapshots = @()
try {
    $process = Start-Process -FilePath $exe -WorkingDirectory $GameDir -PassThru -ArgumentList $arguments
    $deadline = $started.AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline -and -not $process.HasExited) {
        $snapshots = @(Get-ChildItem $GameDir -Filter 'devshot-*.json' | Where-Object {
            if ($_.LastWriteTime -le $started) { return $false }
            try { (Get-Content -Raw -LiteralPath $_.FullName | ConvertFrom-Json).snapshot_run -eq $runId }
            catch { $false } # The writer may still be completing this file.
        })
        $log = Join-Path $GameDir 'devprobe.log'
        $probed = -not $Probe -or ((Test-Path $log) -and (Select-String -Path $log -Pattern 'probe done' -Quiet))
        if ($snapshots.Count -ge $shots -and $probed) { break }
        Start-Sleep -Milliseconds 500
    }
    Start-Sleep -Milliseconds 500
} finally {
    $env:F2CE_DEV_BACKGROUND = $previousBackground
    if ($null -ne $process) {
        if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
        if (-not $process.WaitForExit(10000)) { throw 'The background test process did not exit.' }
    }
}
if ($snapshots.Count -lt $shots) { throw "No snapshot within $TimeoutSeconds s." }
foreach ($snapshot in $snapshots) {
    if ($OutputDirectory) {
        New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
        $metadata = Get-Content -Raw -LiteralPath $snapshot.FullName | ConvertFrom-Json
        Copy-Item -LiteralPath $snapshot.FullName -Destination $OutputDirectory
        Copy-Item -LiteralPath (Join-Path $GameDir $metadata.image) -Destination $OutputDirectory
    }
    Write-Output ("SNAPSHOT: " + $snapshot.FullName)
    Get-Content $snapshot.FullName
}
if ($Probe) {
    Write-Output "PROBE:"
    Get-Content (Join-Path $GameDir 'devprobe.log')
}
