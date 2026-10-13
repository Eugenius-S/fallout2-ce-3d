# Configures and builds the project with Ninja + MSVC 14.40 (v143 toolset).
#
# The VS BuildTools 18.2.1 install on this machine has a partially broken VC
# workload: vswhere does not register the VC.Tools component and vcvars64.bat
# is missing, so VsDevCmd cannot put cl.exe on PATH. The compiler, STL and
# Windows SDK are fully present on disk, so this script sets PATH/INCLUDE/LIB
# explicitly. No stack substitution: same cl.exe, same Windows SDK (22621 =
# spec floor).
param(
    [string]$BuildType = 'Release'
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot

$BtRoot = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools'
$Msvc = Join-Path $BtRoot 'VC\Tools\MSVC\14.40.33807'
$SdkRoot = 'C:\Program Files (x86)\Windows Kits\10'
$SdkVersion = '10.0.22621.0'
foreach ($d in @($Msvc, (Join-Path $SdkRoot "Include\$SdkVersion\um"))) {
    if (!(Test-Path $d)) { throw "Toolchain component missing: $d" }
}

$env:PATH = @(
    (Join-Path $Msvc 'bin\Hostx64\x64'),
    (Join-Path $SdkRoot "bin\$SdkVersion\x64"),
    'C:\Program Files\CMake\bin',
    'C:\Users\jiraf\AppData\Local\Microsoft\WinGet\Links',
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

$buildDir = Join-Path $ProjectRoot 'build'
$cache = Join-Path $buildDir 'CMakeCache.txt'
if (Test-Path $cache) {
    $cachedGen = (Get-Content $cache | Where-Object { $_ -like 'CMAKE_GENERATOR:INTERNAL=*' }) -replace '^CMAKE_GENERATOR:INTERNAL=', ''
    if ($cachedGen -ne 'Ninja') {
        Remove-Item -LiteralPath $buildDir -Recurse -Force
    }
}

cmake -S $ProjectRoot -B $buildDir -G Ninja "-DCMAKE_BUILD_TYPE=$BuildType"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build $buildDir
exit $LASTEXITCODE
