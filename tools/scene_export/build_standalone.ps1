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

py -3 -m PyInstaller --version 2>$null
if ($LASTEXITCODE -ne 0) {
    py -3 -m pip install pyinstaller
    if ($LASTEXITCODE -ne 0) { throw 'pyinstaller is not available' }
}

$work = Join-Path $OutDir 'work'
$spec = Join-Path $OutDir 'spec'
$entry = Join-Path $PSScriptRoot 'standalone.py'

# The whole exporter tree travels as data, vendor and shapes included;
# standalone.py imports it from the extracted copy at runtime.
py -3 -m PyInstaller --noconfirm --clean --onefile --console `
    --name scene-exporter `
    --distpath $OutDir --workpath $work --specpath $spec `
    --add-data "$PSScriptRoot;scene_export" `
    $entry
if ($LASTEXITCODE -ne 0) { throw 'PyInstaller failed' }

Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $spec -Recurse -Force -ErrorAction SilentlyContinue
Write-Host "standalone exporter: $OutDir\scene-exporter.exe"
