# Real (SendInput) keyboard/mouse for dev-tools acceptance runs. This briefly
# focuses the game window and moves the real cursor - use only for scripted
# acceptance bursts, restoring the previous foreground window/cursor after.
param(
    [Parameter(Mandatory=$true)][string]$Cmd,
    [string]$Key,            # ESC|ENTER|TAB|SPACE|BACKSPACE|UP|DOWN|LEFT|RIGHT|F1..F12|A..Z|0..9|ALT_D|ALT_S
    [int]$X, [int]$Y,        # client coords for Click
    [string]$Text            # sequence of simple chars to type
)
$ErrorActionPreference = 'Stop'
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Win9 { [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd); }
public static class RI {
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern IntPtr FindWindow(string cls, string title);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern bool PostMessageW(IntPtr h, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern uint SendInput(uint n, INPUT[] inputs, int size);
    [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint vk, uint type);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [StructLayout(LayoutKind.Sequential)] public struct MOUSEINPUT { public int dx, dy; public uint mouseData, dwFlags, time; public IntPtr dwExtraInfo; }
    [StructLayout(LayoutKind.Sequential)] public struct KEYBDINPUT { public ushort wVk, wScan; public uint dwFlags, time; public IntPtr dwExtraInfo; }
    [StructLayout(LayoutKind.Explicit)] public struct IU { [FieldOffset(0)] public MOUSEINPUT mi; [FieldOffset(0)] public KEYBDINPUT ki; }
    [StructLayout(LayoutKind.Sequential)] public struct INPUT { public uint type; public IU u; }
    public const uint KEYDOWN = 0; public const uint KEYUP = 2;
    public const uint MOVEDOWN = 1; public const uint LEFTDOWN = 2; public const uint LEFTUP = 4;

    public static void Tap(ushort vk, bool alt) {
        var down = new INPUT(); down.type = 1;
        down.u.ki.wVk = vk; down.u.ki.wScan = (ushort)MapVirtualKey(vk, 0); down.u.ki.dwFlags = KEYDOWN;
        if (alt) {
            var a = new INPUT(); a.type = 1; a.u.ki.wVk = 0x12; a.u.ki.dwFlags = KEYDOWN;
            SendInput(1, new INPUT[] { a }, Marshal.SizeOf(typeof(INPUT)));
        }
        SendInput(1, new INPUT[] { down }, Marshal.SizeOf(typeof(INPUT)));
        var up = new INPUT(); up.type = 1;
        up.u.ki.wVk = vk; up.u.ki.dwFlags = KEYUP;
        SendInput(1, new INPUT[] { up }, Marshal.SizeOf(typeof(INPUT)));
        if (alt) {
            var a2 = new INPUT(); a2.type = 1; a2.u.ki.wVk = 0x12; a2.u.ki.dwFlags = KEYUP;
            SendInput(1, new INPUT[] { a2 }, Marshal.SizeOf(typeof(INPUT)));
        }
    }
    public static void Click(int cx, int cy) {
        SetCursorPos(cx, cy);
        System.Threading.Thread.Sleep(120);
        var d = new INPUT(); d.type = 0; d.u.mi.dwFlags = LEFTDOWN;
        SendInput(1, new INPUT[] { d }, Marshal.SizeOf(typeof(INPUT)));
        var u = new INPUT(); u.type = 0; u.u.mi.dwFlags = LEFTUP;
        SendInput(1, new INPUT[] { u }, Marshal.SizeOf(typeof(INPUT)));
    }
}
"@
[RI]::SetProcessDPIAware() | Out-Null
$p = Get-Process -Name 'fallout2-ce' -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $p) { throw 'fallout2-ce window not found' }
$h = $p.MainWindowHandle
# Foreground-lock makes SetForegroundWindow from a background process a
# silent no-op; minimize+restore activates the window instead.
[Win9]::ShowWindow($h, 6) | Out-Null   # SW_MINIMIZE
Start-Sleep -Milliseconds 150
[Win9]::ShowWindow($h, 9) | Out-Null   # SW_RESTORE
Start-Sleep -Milliseconds 350

$cr = New-Object RI+RECT
[RI]::GetClientRect($h, [ref]$cr) | Out-Null
$origin = New-Object RI+POINT
$origin.X = 0; $origin.Y = 0
[RI]::ClientToScreen($h, [ref]$origin) | Out-Null

switch ($Cmd) {
    'Key' {
        $map = @{ ESC = 0x1B; ENTER = 0x0D; RETURN = 0x0D; TAB = 0x09; SPACE = 0x20; BACKSPACE = 0x08;
                  UP = 0x26; DOWN = 0x28; LEFT = 0x25; RIGHT = 0x27;
                  F1 = 0x70; F2 = 0x71; F3 = 0x72; F4 = 0x73; F5 = 0x74; F6 = 0x75; F7 = 0x76; F8 = 0x77;
                  F9 = 0x78; F10 = 0x79; F11 = 0x7A; F12 = 0x7B; D = 0x44; S = 0x53 }
        $vk = $map[$Key]
        $alt = $false
        if ($vk -eq $null) {
            if ($Key -like 'ALT_*') { $vk = $map[$Key.Substring(4)]; $alt = $true }
        }
        if ($vk -eq $null) { throw "unsupported key $Key" }
        [RI]::Tap([uint16]$vk, $alt)
        Write-Output "key $Key (alt=$alt)"
    }
    'Type' {
        foreach ($ch in $Text.ToCharArray()) {
            $vk = [RI]::MapVirtualKey([uint16][char]::ToUpper($ch), 0)
            $shift = $ch -cmatch '[A-Z]'
            if ($shift) {
                $d = New-Object RI+INPUT; $d.type = 1; $d.u.ki.wVk = 0x10; $d.u.ki.dwFlags = 0
                [RI]::SendInput(1, @($d), [Runtime.InteropServices.Marshal]::SizeOf([type][RI+INPUT]))
            }
            [RI]::Tap([uint16]$vk, $false)
            if ($shift) {
                $u = New-Object RI+INPUT; $u.type = 1; $u.u.ki.wVk = 0x10; $u.u.ki.dwFlags = 2
                [RI]::SendInput(1, @($u), [Runtime.InteropServices.Marshal]::SizeOf([type][RI+INPUT]))
            }
            Start-Sleep -Milliseconds 40
        }
        Write-Output "typed $Text"
    }
    'Click' {
        [RI]::Click($origin.X + $X, $origin.Y + $Y)
        Write-Output "clicked $X,$Y (client)"
    }
    'PostKey' {
        $map = @{ ESC = 0x1B; ENTER = 0x0D; RETURN = 0x0D; TAB = 0x09; SPACE = 0x20; BACKSPACE = 0x08;
                  UP = 0x26; DOWN = 0x28; LEFT = 0x25; RIGHT = 0x27;
                  A = 0x41; B = 0x42; C = 0x43; D = 0x44; E = 0x45; F = 0x46; G = 0x47; H = 0x48;
                  I = 0x49; J = 0x4A; K = 0x4B; L = 0x4C; M = 0x4D; N = 0x4E; O = 0x4F; P = 0x50;
                  Q = 0x51; R = 0x52; S = 0x53; T = 0x54; U = 0x55; V = 0x56; W = 0x57; X = 0x58;
                  Y = 0x59; Z = 0x5A;
                  '0' = 0x30; '1' = 0x31; '2' = 0x32; '3' = 0x33; '4' = 0x34; '5' = 0x35; '6' = 0x36; '7' = 0x37;
                  '8' = 0x38; '9' = 0x39;
                  F9 = 0x78 }
        $alt = $false
        $vk = $map[$Key]
        if ($vk -eq $null -and $Key -like 'ALT_*') { $vk = $map[$Key.Substring(4)]; $alt = $true }
        if ($vk -eq $null) { throw "unsupported key $Key" }
        $scancode = [RI]::MapVirtualKey([uint16]$vk, 0)
        $lparamDn = [IntPtr](1 -bor ([int]$scancode -shl 16))
        $lparamUp = [IntPtr](0xC0000000 -bor ([int]$scancode -shl 16) -bor 1)
        $downMsg = 0x0100
        $upMsg = 0x0101
        if ($alt) { $downMsg = 0x0104; $upMsg = 0x0105 }  # WM_SYSKEYDOWN/UP
        if ($alt) {
            $sc = [RI]::MapVirtualKey(0x12, 0)
            [RI]::PostMessageW($h, 0x0104, [IntPtr]0x12, [IntPtr](1 -bor ([int]$sc -shl 16))) | Out-Null
            Start-Sleep -Milliseconds 40
        }
        [RI]::PostMessageW($h, $downMsg, [IntPtr]$vk, $lparamDn) | Out-Null
        Start-Sleep -Milliseconds 60
        [RI]::PostMessageW($h, $upMsg, [IntPtr]$vk, $lparamUp) | Out-Null
        if ($alt) {
            $sc = [RI]::MapVirtualKey(0x12, 0)
            [RI]::PostMessageW($h, 0x0105, [IntPtr]0x12, [IntPtr](0xC0000000 -bor ([int]$sc -shl 16) -bor 1)) | Out-Null
        }
        Write-Output "posted $Key (alt=$alt)"
    }
    default { throw "unknown cmd $Cmd" }
}
