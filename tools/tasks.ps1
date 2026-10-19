# Prints the task board from the headers of tasks/*.md.
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$rows = foreach ($file in Get-ChildItem (Join-Path $Root 'tasks') -Filter '*.md' | Sort-Object Name) {
    $header = @{}
    $lines = Get-Content $file.FullName
    if ($lines.Count -eq 0 -or $lines[0] -ne '---') { continue }
    for ($i = 1; $i -lt $lines.Count -and $lines[$i] -ne '---'; $i++) {
        if ($lines[$i] -match '^([a-z-]+):\s*(.*?)\s*(#.*)?$') { $header[$Matches[1]] = $Matches[2] }
    }
    [pscustomobject]@{
        id = $header['id']
        status = $header['status']
        owner = $header['owner']
        suggested = $header['suggested']
        claude = $header['claude-only']
        deps = $header['depends-on']
        reviewed = $header['reviewed']
        title = $header['title']
    }
}
$rows | Format-Table -AutoSize
