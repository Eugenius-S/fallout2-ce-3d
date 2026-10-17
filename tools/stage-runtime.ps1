# Stages the three runtime directories (runtime/native, runtime/carrier,
# runtime/neural) with one Release EXE each. carrier/neural additionally get
# the four hash-verified neural-runtime files from the pinned manifest and the
# identical ReShade.ini from the spec (differing only in NeuralUplift).
# Re-running with unexpected files already inside a runtime dir is an error;
# nothing is deleted automatically.

param(
    [string]$Configuration = 'Release',
    [switch]$SkipHashCheck
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
# VS multi-config generators put outputs under build\<Configuration>\;
# Ninja (single-config) puts them in build\ directly.
$BuildExe = Join-Path $ProjectRoot "build\$Configuration\f2native.exe"
if (!(Test-Path $BuildExe)) { $BuildExe = Join-Path $ProjectRoot 'build\f2native.exe' }
if (!(Test-Path $BuildExe)) { throw "EXE not built: run the CMake build first" }

$ManifestPath = Join-Path $ProjectRoot 'docs\baseline\native-prototype-2026-10-12.json'
$Manifest = Get-Content $ManifestPath -Raw | ConvertFrom-Json
$RuntimeSource = $Manifest.runtime_source   # E:\Games\Fallout 2\host64 (read-only source)

$ExpectedIni = [ordered]@{
    carrier = @{ NeuralUplift = 0 }
    neural  = @{ NeuralUplift = 1 }
}

function Write-ReShadeIni([string]$Path, [int]$NeuralUplift) {
    $lines = @(
        '[GENERAL]',
        'PresetPath=.\ReShadePreset.ini',
        'EffectSearchPaths=.\effects',
        'TextureSearchPaths=.\textures',
        '',
        '[RenoDX.DLSS5]',
        'EnableHooks=2',
        "NeuralUplift=$NeuralUplift",
        'NREnableUpscaling=0',
        'NRFollowInputRes=0',
        'NRResolutionScale=1',
        'NRPreUpscale=0',
        'NRPasses=1',
        'NRAutoMask=0',
        'NRIntensity=1'
    )
    [IO.File]::WriteAllLines($Path, $lines)
}

function Assert-FileHash([string]$Path, [string]$Expected) {
    $actual = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -cne $Expected.ToLowerInvariant()) {
        throw "SHA256 mismatch: $Path`n  expected $Expected`n  actual   $actual"
    }
}

$staged = @()
foreach ($mode in @('native', 'carrier', 'neural')) {
    $dir = Join-Path $ProjectRoot "runtime\$mode"
    if (Test-Path $dir) {
        $existing = @(Get-ChildItem -LiteralPath $dir -Recurse -File)
        $expectedNames = @('f2native.exe', 'scene.hlsl', 'present.hlsl',
                           'ReShade.log', 'dlss5-feed-host.log') # runtime logs from previous runs
        if ($mode -ne 'native') {
            $expectedNames += $Manifest.runtime_files | ForEach-Object { $_.name }
            if ($mode -eq 'carrier' -or $mode -eq 'neural') { $expectedNames += 'ReShade.ini' }
        }
        foreach ($file in $existing) {
            if ($expectedNames -notcontains $file.Name) {
                throw "runtime\$mode contains unexpected file '$($file.Name)' - inspect manually, nothing deleted"
            }
        }
    } else {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }

    Copy-Item -LiteralPath $BuildExe -Destination (Join-Path $dir 'f2native.exe') -Force
    $shaderDir = Join-Path $dir 'shaders'
    if (Test-Path $shaderDir) { Remove-Item -LiteralPath $shaderDir -Recurse -Force }
    Copy-Item -LiteralPath (Join-Path $ProjectRoot 'shaders') -Destination $shaderDir -Recurse -Force
    $staged += "runtime/$mode/f2native.exe"

    if ($mode -ne 'native') {
        foreach ($entry in $Manifest.runtime_files) {
            $src = Join-Path $RuntimeSource $entry.name
            if (-not (Test-Path $src)) { throw "Missing runtime source: $src" }
            if (-not $SkipHashCheck) { Assert-FileHash $src $entry.sha256 }
            Copy-Item -LiteralPath $src -Destination (Join-Path $dir $entry.name) -Force
            $staged += "runtime/$mode/$($entry.name)"
        }
        New-Item -ItemType Directory -Path (Join-Path $dir 'effects') -Force | Out-Null
        New-Item -ItemType Directory -Path (Join-Path $dir 'textures') -Force | Out-Null
        Write-ReShadeIni (Join-Path $dir 'ReShade.ini') $ExpectedIni[$mode].NeuralUplift
        $staged += "runtime/$mode/ReShade.ini"
    }
    Write-Host "staged runtime/$mode"
}

$inventory = [pscustomobject]@{
    staged_at = (Get-Date).ToUniversalTime().ToString('o')
    configuration = $Configuration
    exe_hash = (Get-FileHash $BuildExe -Algorithm SHA256).Hash.ToLowerInvariant()
    files = $staged
}
[IO.File]::WriteAllText((Join-Path $ProjectRoot 'runtime\staged.json'),
    ($inventory | ConvertTo-Json -Depth 4))
Write-Host "STAGING OK ($($staged.Count) files)"
