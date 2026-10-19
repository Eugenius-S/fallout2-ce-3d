# Prints the task board from the headers of tasks/*.md.
#
# -Next <agent> prints the one task the agent should work on (its own doing
# task first, otherwise the free task by the AGENTS.md rule); -Review lists
# the done tasks that still need a review. Both read the task files from
# origin/main after a git fetch; without a network they fall back to the
# working tree and say so.
param(
    [string]$Next,
    [switch]$Review
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot

function Read-Header([string[]]$lines) {
    $header = @{}
    if ($lines.Count -eq 0 -or $lines[0] -ne '---') { return $null }
    for ($i = 1; $i -lt $lines.Count -and $lines[$i] -ne '---'; $i++) {
        if ($lines[$i] -match '^([a-z-]+):\s*(.*?)\s*(#.*)?$') { $header[$Matches[1]] = $Matches[2] }
    }
    return $header
}

# Prefer origin/main (another agent may work on a branch in another
# worktree); fall back to the working tree without a network.
$source = 'origin/main'
git fetch origin 2>$null
$files = @()
git ls-tree --name-only origin/main tasks/ 2>$null | ForEach-Object { $files += $_ }
if ($files.Count -eq 0) {
    $source = 'the working tree (git fetch failed)'
    $files = Get-ChildItem (Join-Path $Root 'tasks') -Filter '*.md' |
        ForEach-Object { "tasks/" + $_.Name } | Sort-Object
}

$rows = foreach ($file in $files) {
    $lines = git show "$source`:$file" 2>$null
    if ($null -eq $lines -or @($lines).Count -eq 0) {
        $lines = Get-Content (Join-Path $Root ($file -replace '/', '\'))
    }
    $header = Read-Header @($lines)
    if ($null -eq $header) { continue }
    [pscustomobject]@{
        id = $header['id']
        status = $header['status']
        owner = $header['owner']
        suggested = $header['suggested']
        claude = $header['claude-only']
        deps = $header['depends-on']
        reviewed = $header['reviewed']
        title = $header['title']
        file = $file
        branch = 'task/' + (Split-Path -Leaf $file).Replace('.md', '')
    }
}

if ($Review) {
    $rows | Where-Object { $_.status -eq 'done' -and $_.reviewed -eq 'no' } |
        Format-Table -AutoSize id, title, file
    exit 0
}

if ($Next) {
    $agent = $Next
    $mine = @($rows | Where-Object { $_.status -eq 'doing' -and $_.owner -eq $agent } | Sort-Object { [int]$_.id })
    $done = @{}
    foreach ($r in $rows) { if ($r.status -eq 'done') { $done[$r.id] = $true } }

    $task = $null
    if ($mine.Count -gt 0) {
        $task = $mine[0]
        Write-Output "${agent}: continuing its own doing task"
    } else {
        $free = @($rows | Where-Object {
            $_.status -eq 'todo' `
                -and ($_.claude -ne 'yes' -or $agent -eq 'claude') `
                -and (($_.deps -replace '[\[\]]', '').Split(',') | Where-Object { $_.Trim() } |
                    ForEach-Object { $done[$_.Trim()] }) -notcontains $false
        })
        $suggested = @($free | Where-Object { $_.suggested -eq $agent } | Sort-Object { [int]$_.id })
        if ($suggested.Count -gt 0) {
            $task = $suggested[0]
        } elseif ($free.Count -gt 0) {
            $task = @($free | Sort-Object { [int]$_.id })[0]
        }
        if ($task) {
            Write-Output "${agent}: next free task"
        }
    }

    if ($task) {
        Write-Output "  file:   $($task.file)"
        Write-Output "  branch: $($task.branch)"
        Write-Output "  title:  $($task.title)"
    } else {
        Write-Output "${agent}: nothing is free."
        $waiting = @($rows | Where-Object {
            $_.status -eq 'todo' -and ($_.claude -ne 'yes' -or $agent -eq 'claude')
        } | Sort-Object { [int]$_.id })
        foreach ($t in $waiting) {
            $missing = @(($t.deps -replace '[\[\]]', '').Split(',') |
                Where-Object { $_.Trim() -and !$done[$_.Trim()] } |
                ForEach-Object { $_.Trim() })
            if ($missing.Count -gt 0) {
                Write-Output "  $($t.id) '$($t.title)' waits for: $($missing -join ', ')"
            }
        }
    }
    exit 0
}

$rows | Format-Table -AutoSize
