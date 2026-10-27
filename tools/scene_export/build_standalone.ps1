# Packs the scene exporter as a standalone Windows exe (PyInstaller onefile)
# for the rolling latest release (task 091): the player needs no Python.
#
# The whole tools/scene_export tree travels as data (see standalone.py for
# why), so the resulting exe only needs --game-root to run:
#   scene-exporter.exe --game-root <game> --maps klamath
param(
    [string]$OutDir = ''
)
$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if ($OutDir -eq '') { $OutDir = Join-Path $Root 'build\exporter-standalone' }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

# Prefer the interpreter the environment prepared: setup-python (CI) exports
# pythonLocation and installs PyInstaller into that Python, while the bare
# `py` launcher resolves to the machine's newest Python, which may have none.
# With no pythonLocation (a dev machine) fall back to `py -3`.
$python = 'py'
$pythonArgs = @('-3')
if ($env:pythonLocation -and (Test-Path (Join-Path $env:pythonLocation 'python.exe'))) {
    $python = Join-Path $env:pythonLocation 'python.exe'
    $pythonArgs = @()
}
Write-Host "interpreter: $python $($pythonArgs -join ' ')"

& $python @pythonArgs -m PyInstaller --version 2>$null
if ($LASTEXITCODE -ne 0) {
    & $python @pythonArgs -m pip install pyinstaller
    if ($LASTEXITCODE -ne 0) { throw 'pyinstaller is not available' }
}

$work = Join-Path $OutDir 'work'
$spec = Join-Path $OutDir 'spec'
$entry = Join-Path $PSScriptRoot 'standalone.py'

# The whole exporter tree travels as data, vendor and shapes included;
# standalone.py imports it from the extracted copy at runtime.
& $python @pythonArgs -m PyInstaller --noconfirm --clean --onefile --console `
    --name scene-exporter `
    --distpath $OutDir --workpath $work --specpath $spec `
    --add-data "$PSScriptRoot;scene_export" `
    $entry
if ($LASTEXITCODE -ne 0) { throw 'PyInstaller failed' }

Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $spec -Recurse -Force -ErrorAction SilentlyContinue
Write-Host "standalone exporter: $OutDir\scene-exporter.exe"
