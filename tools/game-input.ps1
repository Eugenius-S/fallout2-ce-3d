# Non-intrusive automation of the fallout2-ce window.
# The game runs as a normal window (parked in the bottom-right corner of the
# work area, never activated, never topmost). Input is delivered as window
# messages (PostMessage), so the user's real cursor and keyboard focus are
# never touched. Screenshots use PrintWindow, which works even when the
# window is partially covered.
#
# Commands:
#   -Shot <file.png>            save window screenshot
#   -Click <x> <y>              left click at client coords
#   -Key <name>                 ESC | ENTER | TAB | SPACE | F1..F12 | UP/DOWN/LEFT/RIGHT | single char
#   -Info                       print window info
param(
    [Parameter(Mandatory=$true)][string]$Cmd,
    [string]$Path,
    [int]$X, [int]$Y,
    [string]$Key
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Win {
    [DllImport("user32.dll")] public static extern IntPtr FindWindow(string cls, string title);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, UIntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint vk, uint type);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint flags);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
}
"@
[Win]::SetProcessDPIAware() | Out-Null

$proc = Get-Process -Name 'fallout2-ce' -ErrorAction SilentlyContinue |
    Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
$h = if ($proc) { $proc.MainWindowHandle } else { [IntPtr]::Zero }
if ($h -eq [IntPtr]::Zero) { $h = [Win]::FindWindow($null, 'FALLOUT II') }
if ($h -eq [IntPtr]::Zero) { throw 'fallout2-ce window not found (not running?)' }

$wr = New-Object Win+RECT
[Win]::GetWindowRect($h, [ref]$wr) | Out-Null
$ww = $wr.R - $wr.L; $wh = $wr.B - $wr.T

function Send-Mouse([uint32]$msg, [int]$cx, [int]$cy) {
    $lp = [IntPtr](($cy -shl 16) -bor ($cx -band 0xFFFF))
    $wp = [UIntPtr]0
    if ($msg -eq 0x201) { $wp = [UIntPtr]1 }   # MK_LBUTTON
    [Win]::PostMessage($h, $msg, $wp, $lp) | Out-Null
}
function Send-VK([uint32]$vk) {
    $sc = [Win]::MapVirtualKey($vk, 0)
    $dn = [IntPtr](1 -bor ($sc -shl 16))
    $up = [IntPtr](0xC0000000 -bor ($sc -shl 16) -bor 1)
    [Win]::PostMessage($h, 0x0100, [UIntPtr]$vk, $dn) | Out-Null
    Start-Sleep -Milliseconds 60
    [Win]::PostMessage($h, 0x0101, [UIntPtr]$vk, $up) | Out-Null
}

switch ($Cmd) {
    'Info' { Write-Output "window=${ww}x${wh} at $($wr.L),$($wr.T)" }
    'Park' {
        Add-Type -AssemblyName System.Windows.Forms
        $wa = [System.Windows.Forms.SystemInformation]::WorkingArea
        $x = $wa.Left + $wa.Width - $ww - 8
        $y = $wa.Top + $wa.Height - $wh - 8
        # SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE
        [Win]::SetWindowPos($h, [IntPtr]0, $x, $y, 0, 0, 0x0003) | Out-Null
        Write-Output "parked at $x,$y"
    }
    'Shot' {
        $bmp = New-Object System.Drawing.Bitmap $ww, $wh
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $hdc = $g.GetHdc()
        [Win]::PrintWindow($h, $hdc, 2) | Out-Null   # PW_RENDERFULLCONTENT
        $g.ReleaseHdc($hdc)
        $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
        Write-Output "saved $Path (${ww}x${wh})"
    }
    'Click' {
        Send-Mouse 0x0200 $X $Y | Out-Null          # WM_MOUSEMOVE
        Start-Sleep -Milliseconds 80
        Send-Mouse 0x0201 $X $Y                      # WM_LBUTTONDOWN
        Start-Sleep -Milliseconds 60
        Send-Mouse 0x0202 $X $Y                      # WM_LBUTTONUP
        Write-Output "clicked $X,$Y"
    }
    'Key' {
        $map = @{ ESC=0x1B; ENTER=0x0D; RETURN=0x0D; TAB=0x09; SPACE=0x20;
                  UP=0x26; DOWN=0x28; LEFT=0x25; RIGHT=0x27; BACKSPACE=0x08 }
        $vk = 0
        if ($map.ContainsKey($Key.ToUpper())) { $vk = $map[$Key.ToUpper()] }
        elseif ($Key -match '^[a-z0-9]$') { $vk = [Console]::OutputEncoding.GetBytes($Key.ToUpper())[0] }
        elseif ($Key -match '^F([1-9]|1[0-2])$') { $vk = 0x6F + [int]$Matches[1] }
        else { throw "unsupported key '$Key'" }
        Send-VK $vk
        Write-Output "sent vk=$vk ($Key)"
    }
    default { throw "unknown command $Cmd" }
}
