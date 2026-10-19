# Configures and builds the pinned fallout2-ce (subdirectory `ce`) with
# Ninja + MSVC into `build/`.
#
# Toolchain selection:
# 1. The owner's machine: VS BuildTools 18 with a partially broken VC
#    workload (vswhere does not register the VC.Tools component and
#    vcvars64.bat is missing), so the script sets PATH/INCLUDE/LIB explicitly
#    from the on-disk MSVC 14.40 and Windows SDK 22621.
# 2. Any other machine (e.g. CI): when those hard-coded paths are missing,
#    fall back to the standard Visual Studio environment via vswhere +
#    vcvars64.bat.
param(
    [string]$BuildType = 'Release'
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$SourceDir = Join-Path $ProjectRoot 'ce'

$BtRoot = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools'
$Msvc = Join-Path $BtRoot 'VC\Tools\MSVC\14.40.33807'
$SdkRoot = 'C:\Program Files (x86)\Windows Kits\10'
$SdkVersion = '10.0.22621.0'
$OwnerLayout = (Test-Path $Msvc) -and (Test-Path (Join-Path $SdkRoot "Include\$SdkVersion\um"))

if ($OwnerLayout) {
    $env:PATH = @(
        (Join-Path $Msvc 'bin\Hostx64\x64'),
        (Join-Path $SdkRoot "bin\$SdkVersion\x64"),
        'C:\Program Files\CMake\bin',
        $env:PATH
    ) -join ';'

    $env:INCLUDE = @(
        (Join-Path $Msvc 'include'),
        (Join-Path $SdkRoot "Include\$SdkVersion\ucrt"),
        (Join-Path $SdkRoot "Include\$SdkVersion\um"),
        (Join-Path $SdkRoot "Include\$SdkVersion\shared"),
        (Join-Path $SdkRoot "Include\$SdkVersion\winrt"),
        (Join-Path $SdkRoot "Include\$SdkVersion\cppwinrt")
    ) -join ';'

    $env:LIB = @(
        (Join-Path $Msvc 'lib\x64'),
        (Join-Path $SdkRoot "Lib\$SdkVersion\ucrt\x64"),
        (Join-Path $SdkRoot "Lib\$SdkVersion\um\x64")
    ) -join ';'
} else {
    # Standard Visual Studio environment: locate an installation with the
    # C++ toolset and import the environment vcvars64 sets.
    $vsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (!(Test-Path $vsWhere)) { throw "Toolchain component missing: $vsWhere" }
    $vsRoot = & $vsWhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath | Select-Object -First 1
    if (!$vsRoot) { throw 'No Visual Studio installation with the C++ toolset found' }
    $vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars64.bat'
    if (!(Test-Path $vcvars)) { throw "Toolchain component missing: $vcvars" }

    # Import the environment vcvars64 sets (PATH, INCLUDE, LIB, ...).
    cmd /s /c "`"$vcvars`" -arch=x64 && set" | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$' -and $matches[1] -ne '') {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
        }
    }
}

$buildDir = Join-Path $ProjectRoot 'build'
$cache = Join-Path $buildDir 'CMakeCache.txt'
if (Test-Path $cache) {
    $cachedGen = (Get-Content $cache | Where-Object { $_ -like 'CMAKE_GENERATOR:INTERNAL=*' }) -replace '^CMAKE_GENERATOR:INTERNAL=', ''
    if ($cachedGen -ne 'Ninja') {
        Remove-Item -LiteralPath $buildDir -Recurse -Force
    }
}

# fallout2-ce defines _STATIC_CPPLIB (static STL + dynamic CRT). With CMake's
# default MSVC_RUNTIME_LIBRARY (DLL) that combination fails LNK2038/LNK1319:
# libcpmt.lib objects are stamped MT while target objects are MD. CE's intent
# is the static runtime, so set it for every target from the cache instead of
# patching upstream sources.
$runtime = 'MultiThreaded$<$<CONFIG:Debug>:Debug>'
cmake -S $SourceDir -B $buildDir -G Ninja "-DCMAKE_BUILD_TYPE=$BuildType" "-DCMAKE_MSVC_RUNTIME_LIBRARY=$runtime"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build $buildDir
exit $LASTEXITCODE
