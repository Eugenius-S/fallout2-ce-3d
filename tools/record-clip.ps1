# Records a video clip of a separate test game for the showcase video (097).
#
# The fresh build runs as build/record-run/fallout2-ce-record.exe with the
# fixed config from tools/test-config/, beside the owner's game and beside
# dev-snapshot.ps1 runs: its own exe, config and output folder
# (build/record-run/runs/<id>). The window opens off screen like a background
# test run (never activated, so it never steals focus or pauses), then is
# moved on screen, kept topmost without activation while ffmpeg grabs its
# client area, and closed when the clip is done.
#
# -Keys drives the game during the clip with window messages (no real cursor,
# no focus): "2:F,1.5:RIGHT,1:ctrl+F10" waits 2 s and presses F, waits 1.5 s
# and presses Right, waits 1 s and presses Ctrl+F10. Times count from the
# start of the recording. "2:W~3" holds W for 3 s (the fly camera reads the
# held keys); the next step waits for the release.
# -Config sets config keys for the clip: 'camera_rotation=1'.
# -GameArgs passes extra dev flags (e.g. '--dev-fly','--dev-fly-turn=30').
#
# The clip goes to game/shots/097/<Name>.mp4 (H.264, 30 fps, no sound).
param(
    [Parameter(Mandatory = $true)][string]$Name,
    [string]$Map = 'kladwtwn',
    [int]$Elevation = 0,
    [int]$ScreenWidth = 1920,
    [int]$ScreenHeight = 1080,
    [int]$Scale = 2,
    [double]$Seconds = 10,
    # Seconds between the window showing up and the recording start (map load).
    [double]$Warmup = 12,
    [string]$Keys = '',
    [string[]]$GameArgs = @(),
    [string[]]$Config = @(),
    # The game text language; the video is for the GitHub page.
    [ValidateSet('english', 'russian')]
    [string]$Language = 'english',
    [switch]$KeepRunFolder
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$GameDir = Join-Path $Root 'game'
$built = Join-Path $Root 'build\fallout2-ce.exe'
if (-not (Test-Path $built)) { throw "No build at $built; run tools/configure-build.ps1 first." }
if (-not (Get-Command ffmpeg -ErrorAction SilentlyContinue)) { throw 'ffmpeg is not on PATH.' }
if (($ScreenWidth % $Scale) -ne 0 -or ($ScreenHeight % $Scale) -ne 0 -or
    ($ScreenWidth / $Scale) -lt 640 -or ($ScreenHeight / $Scale) -lt 480) {
    throw 'The scale must divide the screen and leave at least a 640x480 logical framebuffer.'
}

Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class RecWin {
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint flags);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint vk, uint type);
    public delegate bool EnumProc(IntPtr h, IntPtr l);
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc f, IntPtr l);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
    // The background window skips the taskbar, so .NET does not see it as
    // the process main window; look it up among the top-level windows.
    public static IntPtr FindProcessWindow(uint pid) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((h, l) => { uint p; GetWindowThreadProcessId(h, out p);
            if (p == pid && IsWindowVisible(h)) { found = h; return false; } return true; }, IntPtr.Zero);
        return found;
    }
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
}
"@
[RecWin]::SetProcessDPIAware() | Out-Null

# Parse the key script up front so a typo fails before the game starts.
$keyMap = @{ ESC = 0x1B; ENTER = 0x0D; TAB = 0x09; SPACE = 0x20; UP = 0x26; DOWN = 0x28;
    LEFT = 0x25; RIGHT = 0x27; PGUP = 0x21; PGDN = 0x22; HOME = 0x24; END = 0x23; BACKSPACE = 0x08;
    LBRACKET = 0xDB; RBRACKET = 0xDD }
$steps = @()
$at = 0.0
foreach ($item in ($Keys -split ',' | Where-Object { $_.Trim() })) {
    $parts = $item.Trim() -split ':', 2
    if ($parts.Count -ne 2) { throw "Key step '$item' is not wait:key." }
    $at += [double]::Parse($parts[0], [System.Globalization.CultureInfo]::InvariantCulture)
    $mods = @()
    $keyName = $parts[1].ToUpperInvariant()
    $hold = 0.06
    if ($keyName -match '^(.+)~([0-9.]+)$') {
        $keyName = $Matches[1]
        $hold = [double]::Parse($Matches[2], [System.Globalization.CultureInfo]::InvariantCulture)
    }
    while ($keyName -match '^(CTRL|ALT|SHIFT)\+(.+)$') { $mods += $Matches[1]; $keyName = $Matches[2] }
    $vk = if ($keyMap.ContainsKey($keyName)) { $keyMap[$keyName] }
        elseif ($keyName -match '^F([1-9]|1[0-2])$') { 0x6F + [int]$Matches[1] }
        elseif ($keyName -match '^[A-Z0-9]$') { [int][char]$keyName }
        else { throw "Unsupported key '$keyName'." }
    $steps += [pscustomobject]@{ At = $at; Vk = $vk; Mods = $mods; Hold = $hold }
}

$exeDirectory = Join-Path $Root 'build\record-run'
New-Item -ItemType Directory -Path $exeDirectory -Force | Out-Null
$exe = Join-Path $exeDirectory 'fallout2-ce-record.exe'
if (Get-Process fallout2-ce-record -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $exe }) {
    throw "A recording game is already running from $exe."
}
$config = [System.IO.File]::ReadAllText((Join-Path $PSScriptRoot 'test-config\fallout2.cfg'))
$config = [regex]::Replace($config, '(?m)^language=.*$', "language=$Language")
foreach ($pair in $Config) {
    $key, $value = $pair -split '=', 2
    if (-not [regex]::IsMatch($config, "(?m)^$([regex]::Escape($key))=")) { throw "No config key '$key' in tools/test-config/fallout2.cfg." }
    $config = [regex]::Replace($config, "(?m)^$([regex]::Escape($key))=.*$", "$key=$value")
}
[System.IO.File]::WriteAllText((Join-Path $exeDirectory 'fallout2.cfg'), $config, [System.Text.UTF8Encoding]::new($false))
if (-not (Test-Path $exe) -or (Get-FileHash $built).Hash -ne (Get-FileHash $exe).Hash) { Copy-Item $built $exe -Force }
$runId = [guid]::NewGuid().ToString('N').Substring(0, 12)
$runDirectory = Join-Path $exeDirectory "runs\$runId"
New-Item -ItemType Directory -Path $runDirectory -Force | Out-Null
# The game loads game/mods/mods_order.txt from its working directory, and the
# owner's list carries the Russian Restoration Project packs, which replace
# the interface art (PUNCH, SKILLDEX, ...) with Russian lettering. An English
# clip runs from build/record-run/game instead: junctions and hard links to
# every entry of game/ except mods/, whose own copy links the same files with
# a mods_order.txt that leaves the *_russian packs out. The owner's files are
# never written. Links are only added, never removed: deleting a junction
# recursively would delete the owner's files behind it.
$workDir = $GameDir
if ($Language -eq 'english') {
    $workDir = Join-Path $exeDirectory 'game'
    New-Item -ItemType Directory -Path (Join-Path $workDir 'mods') -Force | Out-Null
    foreach ($entry in Get-ChildItem -LiteralPath $GameDir -Force) {
        if ($entry.Name -eq 'mods') { continue }
        $link = Join-Path $workDir $entry.Name
        if (Test-Path -LiteralPath $link) { continue }
        if ($entry.PSIsContainer) { New-Item -ItemType Junction -Path $link -Target $entry.FullName | Out-Null }
        else { New-Item -ItemType HardLink -Path $link -Target $entry.FullName | Out-Null }
    }
    foreach ($entry in Get-ChildItem -LiteralPath (Join-Path $GameDir 'mods') -File) {
        if ($entry.Name -eq 'mods_order.txt') { continue }
        $link = Join-Path $workDir "mods\$($entry.Name)"
        if (-not (Test-Path -LiteralPath $link)) { New-Item -ItemType HardLink -Path $link -Target $entry.FullName | Out-Null }
    }
    $order = Get-Content -LiteralPath (Join-Path $GameDir 'mods\mods_order.txt') | Where-Object { $_ -notmatch '_russian' }
    [System.IO.File]::WriteAllLines((Join-Path $workDir 'mods\mods_order.txt'), [string[]]$order)
}
$outDirectory = Join-Path $GameDir 'shots\097'
New-Item -ItemType Directory -Path $outDirectory -Force | Out-Null
$clip = Join-Path $outDirectory "$Name.mp4"

$arguments = @('--dev-tools', "--dev-screen=${ScreenWidth}x${ScreenHeight}", "--dev-scale=$Scale",
    '--dev-new-game', "--dev-map=$Map", "--dev-elevation=$Elevation") + $GameArgs
$saved = @{ B = $env:F2CE_DEV_BACKGROUND; O = $env:F2CE_DEV_OUT; S = $env:F2CE_SCENE_CACHE }
$env:F2CE_DEV_BACKGROUND = '1'
$env:F2CE_DEV_OUT = $runDirectory
$cache = Join-Path $Root 'build\scene-cache'
if (Test-Path -LiteralPath $cache -PathType Container) { $env:F2CE_SCENE_CACHE = $cache }
$process = $null
try {
    $process = Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory $workDir -PassThru
    $deadline = (Get-Date).AddSeconds(60)
    while (([RecWin]::FindProcessWindow([uint32]$process.Id)) -eq [IntPtr]::Zero) {
        if ($process.HasExited) { throw "The game exited with code $($process.ExitCode) before showing a window." }
        if ((Get-Date) -gt $deadline) { throw 'The game window did not show up within 60 s.' }
        Start-Sleep -Milliseconds 250
    }
    $h = [RecWin]::FindProcessWindow([uint32]$process.Id)
    Start-Sleep -Seconds $Warmup
    if ($process.HasExited) { throw "The game exited with code $($process.ExitCode) during warmup." }

    # On screen at the bottom right of the primary work area, topmost, not
    # activated (SWP_NOSIZE | SWP_NOACTIVATE).
    Add-Type -AssemblyName System.Windows.Forms
    $wa = [System.Windows.Forms.Screen]::PrimaryScreen.WorkingArea
    $wr = New-Object RecWin+RECT
    [RecWin]::GetWindowRect($h, [ref]$wr) | Out-Null
    $x = [math]::Max($wa.Left, $wa.Right - ($wr.R - $wr.L))
    $y = [math]::Max($wa.Top, $wa.Bottom - ($wr.B - $wr.T))
    [RecWin]::SetWindowPos($h, [IntPtr](-1), $x, $y, 0, 0, 0x0011) | Out-Null
    Start-Sleep -Milliseconds 500
    $cr = New-Object RecWin+RECT
    [RecWin]::GetClientRect($h, [ref]$cr) | Out-Null
    $origin = New-Object RecWin+POINT
    [RecWin]::ClientToScreen($h, [ref]$origin) | Out-Null
    $w = ($cr.R - $cr.L) - (($cr.R - $cr.L) % 2)
    $hh = ($cr.B - $cr.T) - (($cr.B - $cr.T) % 2)

    # ffmpeg is not DPI aware; the compatibility layer makes gdigrab use the
    # same physical pixels as the window rectangle above.
    $env:__COMPAT_LAYER = 'HighDpiAware'
    $secondsText = $Seconds.ToString([System.Globalization.CultureInfo]::InvariantCulture)
    $ffArgs = @('-hide_banner', '-loglevel', 'error', '-y', '-f', 'gdigrab', '-framerate', '30',
        '-offset_x', "$($origin.X)", '-offset_y', "$($origin.Y)", '-video_size', "${w}x${hh}",
        '-draw_mouse', '0', '-i', 'desktop', '-t', $secondsText,
        '-c:v', 'libx264', '-preset', 'veryfast', '-crf', '16', '-pix_fmt', 'yuv420p', $clip)
    $ffmpeg = Start-Process ffmpeg -ArgumentList $ffArgs -PassThru -NoNewWindow
    $null = $ffmpeg.Handle # without a cached handle ExitCode stays empty
    Remove-Item Env:__COMPAT_LAYER
    $start = Get-Date
    foreach ($step in $steps) {
        while (((Get-Date) - $start).TotalSeconds -lt $step.At) { Start-Sleep -Milliseconds 20 }
        $modVk = @{ CTRL = 0x11; ALT = 0x12; SHIFT = 0x10 }
        foreach ($m in $step.Mods) { [RecWin]::PostMessage($h, 0x0100, [IntPtr]$modVk[$m], [IntPtr]1) | Out-Null }
        $sc = [RecWin]::MapVirtualKey([uint32]$step.Vk, 0)
        [RecWin]::PostMessage($h, 0x0100, [IntPtr]$step.Vk, [IntPtr](1 -bor ($sc -shl 16))) | Out-Null
        Start-Sleep -Milliseconds ([int]($step.Hold * 1000))
        [RecWin]::PostMessage($h, 0x0101, [IntPtr]$step.Vk, [IntPtr]([int64]0xC0000001 -bor ($sc -shl 16))) | Out-Null
        foreach ($m in $step.Mods) { [RecWin]::PostMessage($h, 0x0101, [IntPtr]$modVk[$m], [IntPtr]([int64]0xC0000001)) | Out-Null }
    }
    $ffmpeg.WaitForExit()
    if ($ffmpeg.ExitCode -ne 0) { throw "ffmpeg failed with code $($ffmpeg.ExitCode)." }
    Write-Output "clip $clip (${w}x${hh}, $Seconds s)"
} finally {
    if ($process -and -not $process.HasExited) { Stop-Process -Id $process.Id -Force }
    $env:F2CE_DEV_BACKGROUND = $saved.B
    $env:F2CE_DEV_OUT = $saved.O
    $env:F2CE_SCENE_CACHE = $saved.S
    if (-not $KeepRunFolder) { Remove-Item -LiteralPath $runDirectory -Recurse -Force -ErrorAction SilentlyContinue }
}
