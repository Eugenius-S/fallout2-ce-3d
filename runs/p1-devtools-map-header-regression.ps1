$ErrorActionPreference = 'Stop'
$mapPath = Join-Path $PSScriptRoot '..\game\data\MAPS\KLADWTWN.SAV'
$bytes = [IO.File]::ReadAllBytes([IO.Path]::GetFullPath($mapPath))
function Read-BigEndianInt32([byte[]]$data, [int]$offset) {
    return [BitConverter]::ToInt32([byte[]]@($data[$offset + 3], $data[$offset + 2], $data[$offset + 1], $data[$offset]), 0)
}
function Assert-Equal([string]$name, $actual, $expected) {
    if ($actual -ne $expected) { throw "$name mismatch: got $actual, expected $expected" }
}
$version = Read-BigEndianInt32 $bytes 0
if ($version -notin 19, 20) { throw "unsupported fixture version: $version" }
Assert-Equal 'fixture version' $version 20
$headerName = [Text.Encoding]::ASCII.GetString($bytes, 4, 16).Trim([char]0)
$normalizedHeaderName = $headerName -replace '(?i)\.(map|sav)$', ''
Assert-Equal 'normalized header map name' $normalizedHeaderName 'KLADWTWN'
$normalizedMapName = 'KLADWTWN.MAP' -replace '(?i)\.(map|sav)$', ''
Assert-Equal 'normalized requested map name' $normalizedMapName $normalizedHeaderName
$localCount = Read-BigEndianInt32 $bytes 32
$scriptIndex = Read-BigEndianInt32 $bytes 36
$flags = Read-BigEndianInt32 $bytes 40
$globalCount = Read-BigEndianInt32 $bytes 48
Assert-Equal 'script index offset' $scriptIndex 219
Assert-Equal 'header flags offset' $flags 13
$oldScanStart = 228 # dirty implementation consumed only 6 ints after the name
$headerEnd = 236 # mapHeaderRead consumes field_3C[44] after lastVisitTime
$squareDataStart = $headerEnd + 4 * ($globalCount + $localCount) # mapLoad reads globals, then locals
Assert-Equal 'serialized elevations are only 0' ($flags -band 0x0E) 0x0C
if ($oldScanStart -ge $headerEnd -or $oldScanStart -ge $squareDataStart) {
    throw "dirty scan unexpectedly reaches square data: $oldScanStart vs header $headerEnd and squares $squareDataStart"
}
"PASS: $headerName v$version normalizes to $normalizedHeaderName; flags=0x$('{0:X2}' -f $flags) serialize elevation 0 only. Dirty scan starts at byte $oldScanStart inside the header; elevation square data starts at byte $squareDataStart after vars ($globalCount global + $localCount local)."
