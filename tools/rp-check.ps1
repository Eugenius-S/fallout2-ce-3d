param(
    [Parameter(Mandatory = $true)]
    [string]$GameRoot,
    [Parameter(Mandatory = $true)]
    [string]$BuildExecutable,
    [string[]]$Maps = @('artemple', 'kladwtwn', 'denres1', 'abbey', 'epamain1', 'slvcamp')
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$gamePath = [System.IO.Path]::GetFullPath($GameRoot)
$buildPath = if ([System.IO.Path]::IsPathRooted($BuildExecutable)) {
    [System.IO.Path]::GetFullPath($BuildExecutable)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $root $BuildExecutable))
}
$mainWorktree = (git -C $root worktree list --porcelain | Select-Object -First 1) -replace '^worktree ', ''
$ownerGamePath = [System.IO.Path]::GetFullPath((Join-Path $mainWorktree 'game'))
if ($gamePath.TrimEnd('\') -ieq $ownerGamePath.TrimEnd('\')) {
    throw 'GameRoot must be an isolated scratch copy, not the main checkout game folder.'
}
if (-not (Test-Path -LiteralPath $gamePath -PathType Container)) { throw "No game root: $gamePath" }
if (-not (Test-Path -LiteralPath (Join-Path $gamePath 'ddraw.ini') -PathType Leaf)) {
    throw 'GameRoot has no ddraw.ini; this does not look like an installed RP copy.'
}
if (-not (Test-Path -LiteralPath (Join-Path $gamePath 'mods\rpu.dat') -PathType Leaf)) {
    throw 'GameRoot has no mods/rpu.dat; this does not look like an installed RP copy.'
}
if (-not (Test-Path -LiteralPath $buildPath -PathType Leaf)) { throw "No CE executable: $buildPath" }
if ($Maps.Count -eq 0) { throw 'Specify at least one map.' }

$runId = [guid]::NewGuid().ToString('N')
$relativeCandidate = "build/rp-check-candidates/$runId/fallout2-ce.exe"
$candidate = Join-Path $root ($relativeCandidate -replace '/', '\')
$testDirectory = "build/rp-check-test/$runId"
$resultsRoot = Join-Path $root ("build/rp-check-runs/$runId" -replace '/', '\')
New-Item -ItemType Directory -Path (Split-Path -Parent $candidate), $resultsRoot -Force | Out-Null
Copy-Item -LiteralPath $buildPath -Destination $candidate

$snapshotScript = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$results = [System.Collections.Generic.List[object]]::new()
foreach ($map in $Maps) {
    if ($map -notmatch '^[A-Za-z0-9_-]+$') { throw "Invalid map name: $map" }
    $mapOutput = Join-Path $resultsRoot $map
    $relativeOutput = "build/rp-check-runs/$runId/$map"
    $snapshotArgs = @(
        '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $snapshotScript,
        '-Map', $map, '-Classic', '-CaptureConsole',
        '-GameRoot', $gamePath,
        '-BuildExecutable', $relativeCandidate,
        '-TestDirectory', "$testDirectory-$map",
        '-OutputDirectory', $relativeOutput,
        '-TimeoutSeconds', '90', '-WaitMinutes', '0'
    )
    try {
        & powershell.exe @snapshotArgs
        if ($LASTEXITCODE -ne 0) { throw "dev-snapshot exited with code $LASTEXITCODE" }
    } catch {
        throw "RP boot check failed for map '$map': $($_.Exception.Message). Output: $mapOutput"
    }

    $sidecars = @(Get-ChildItem -LiteralPath $mapOutput -Filter 'devshot-*.json' -File -ErrorAction SilentlyContinue)
    if ($sidecars.Count -ne 1) { throw "Expected one snapshot sidecar for '$map'; found $($sidecars.Count). Output: $mapOutput" }
    $metadata = Get-Content -Raw -LiteralPath $sidecars[0].FullName | ConvertFrom-Json
    if ([string]$metadata.map -ine "$($map.ToUpperInvariant()).MAP") {
        throw "Requested '$map' but the loaded snapshot reports '$($metadata.map)'. Output: $mapOutput"
    }
    $results.Add([pscustomobject]@{
        map = [string]$metadata.map
        loaded = $true
        ce_commit = [string]$metadata.ce_commit
        exe_sha256 = [string]$metadata.exe_sha256
    })
}

$summary = [pscustomobject]@{
    result = 'pass'
    maps = @($results)
}
$summary | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $resultsRoot 'result.json') -Encoding utf8
Write-Output "RP_CHECK PASS maps=$($results.Count) output=build/rp-check-runs/$runId"
