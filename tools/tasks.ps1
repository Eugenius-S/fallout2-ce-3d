# Prints the task board from the headers of tasks/*.md.
#
# -Next <agent> prints the one task the agent should work on (its own doing
# task first, otherwise the free task by the AGENTS.md rule); -Review lists
# the done tasks that still need a review. Both read the task files from
# origin/main after a git fetch; without a network they fall back to the
# working tree and say so. A task is taken when its branch task/NNN-slug is
# on origin: its header is read from that branch. A doing task whose branch
# has had no commit for three days is stale: any agent may continue it.
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
# git's warnings on stderr must not stop Windows PowerShell 5 here.
$ErrorActionPreference = 'Continue'
git fetch origin 2>$null
$ErrorActionPreference = 'Stop'
$files = @()
git ls-tree --name-only origin/main tasks/ 2>$null | ForEach-Object { $files += $_ }
if ($files.Count -eq 0) {
    $source = 'the working tree (git fetch failed)'
    $files = Get-ChildItem (Join-Path $Root 'tasks') -Filter '*.md' |
        ForEach-Object { "tasks/" + $_.Name } | Sort-Object
}

$StaleDays = 3
$branchTimes = @{}
git for-each-ref --format='%(refname:short) %(committerdate:unix)' refs/remotes/origin/task/ 2>$null |
    ForEach-Object { $parts = $_ -split ' '; $branchTimes[$parts[0] -replace '^origin/', ''] = [long]$parts[1] }
$now = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()

$rows = foreach ($file in $files) {
    $branch = 'task/' + (Split-Path -Leaf $file).Replace('.md', '')
    $lines = git show "$source`:$file" 2>$null
    $onMain = Read-Header @($lines)
    if ($branchTimes.ContainsKey($branch) -and $onMain -and $onMain['status'] -ne 'done') {
        $fromBranch = git show "origin/$branch`:$file" 2>$null
        if ($fromBranch) { $lines = $fromBranch }
    }
    if ($null -eq $lines -or @($lines).Count -eq 0) {
        $lines = Get-Content (Join-Path $Root ($file -replace '/', '\'))
    }
    $header = Read-Header @($lines)
    if ($null -eq $header) { continue }
    $status = $header['status']
    if ($status -eq 'todo' -and $branchTimes.ContainsKey($branch)) { $status = 'doing' }
    $stale = $status -eq 'doing' -and (-not $branchTimes.ContainsKey($branch) -or
        $now - $branchTimes[$branch] -gt $StaleDays * 86400)
    [pscustomobject]@{
        id = $header['id']
        status = if ($stale) { 'stale' } else { $status }
        owner = $header['owner']
        suggested = $header['suggested']
        priority = $header['priority']
        claude = $header['claude-only']
        deps = $header['depends-on']
        reviewed = $header['reviewed']
        title = $header['title']
        file = $file
        branch = $branch
    }
}

if ($Review) {
    $rows | Where-Object { $_.status -eq 'done' -and $_.reviewed -eq 'no' } |
        Format-Table -AutoSize id, title, file
    exit 0
}

if ($Next) {
    $agent = $Next
    $mine = @($rows | Where-Object { $_.status -in 'doing', 'stale' -and $_.owner -eq $agent } | Sort-Object { [int]$_.id })
    $stale = @($rows | Where-Object { $_.status -eq 'stale' -and ($_.claude -ne 'yes' -or $agent -eq 'claude') } |
        Sort-Object { [int]$_.id })
    $done = @{}
    foreach ($r in $rows) { if ($r.status -eq 'done') { $done[$r.id] = $true } }

    $task = $null
    if ($mine.Count -gt 0) {
        $task = $mine[0]
        Write-Output "${agent}: continuing its own doing task"
    } elseif ($stale.Count -gt 0) {
        $task = $stale[0]
        Write-Output "${agent}: continuing a stale task of $($task.owner) from its branch and Log"
    } else {
        $free = @($rows | Where-Object {
            $_.status -eq 'todo' `
                -and ($_.claude -ne 'yes' -or $agent -eq 'claude') `
                -and (($_.deps -replace '[\[\]]', '').Split(',') | Where-Object { $_.Trim() } |
                    ForEach-Object { [bool]$done[$_.Trim()] }) -notcontains $false
        })
        # priority: high comes before the rest, then the lower id.
        $order = @({ if ($_.priority -eq 'high') { 0 } else { 1 } }, { [int]$_.id })
        $suggested = @($free | Where-Object { $_.suggested -eq $agent } | Sort-Object $order)
        if ($suggested.Count -gt 0) {
            $task = $suggested[0]
        } elseif ($free.Count -gt 0) {
            $task = @($free | Sort-Object $order)[0]
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
