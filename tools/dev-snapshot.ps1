# Background test run: starts the freshly built game in the 3D scene, off
# screen and without taking focus, takes one metadata snapshot and closes only
# that process. The build runs as build/test-run/fallout2-ce-test.exe with the
# fixed config from tools/test-config/ (the game reads and saves fallout2.cfg
# next to its exe), so the owner's game/fallout2.cfg and game/fallout2-ce.exe
# (dev-launch.ps1 installs new builds) are never read or written. Resources
# stay in game/, which remains the working directory, unless -GameRoot selects
# an isolated scratch install.
#
# Every file the run writes (snapshots, logs, the game's saved maps, automap
# and world map) goes to its own folder, build/test-run/runs/<id> (the exe's
# F2CE_DEV_OUT), so test games from different worktrees run side by side and
# never read each other's output. Only runs from the same worktree queue up,
# since they share the exe and its config. With -OutputDirectory the results
# are copied there and the run folder is removed (-KeepRunFolder keeps it);
# without it the run folder stays and holds the results.
#
# -Probe: the mouse is put on the scene's critters and what the game finds
# under it goes to devprobe.log; two snapshots show the hex cursor and
# the hover icon over a critter, then a left click there is logged until the
# game leaves the scene (a dialog).
#
# -Tour "klatrap:0,kladwtwn:0:20100": after the snapshot on -Map, the same
# process travels to each listed map elevation (and view hex) and takes one
# snapshot there. -Classic: the same in the game's own 2D picture.
#
# Prints the snapshot JSON; the image lies next to it (a classic capture's in
# the run folder's shots/game/).
# Scenes come from build/scene-cache when that folder exists (a worktree
# exports there with export.py --cache-root build/scene-cache), so a game
# running from game/ keeps its own mod-cache. -SceneCache DIR reads them from
# DIR instead (the fly tour's cache sides, tools/tour/tour.py).
#
# F2CE_NO_EGG=1 in the environment hides the hero's see-through circle, so an
# egg on/off capture pair shows exactly what the circle cuts.
param(
    [string]$Map = "kladwtwn",
    [int]$Elevation = 0,
    [int]$TimeoutSeconds = 90,
    [ValidateRange(1, 3600)]
    [int]$StepTimeoutSeconds = 30,
    # How long to wait for this worktree's previous test game to finish.
    [ValidateRange(0, 240)]
    [int]$WaitMinutes = 30,
    [switch]$Probe,
    [switch]$SavePreviewFixture,
    [switch]$ExistingSavePreviewFixture,
    [switch]$DialogFixture,
    [switch]$DialogPageInspect,
    [switch]$DialogCursorCheck,
    [switch]$DialogLayoutCheck,
    [ValidateSet('inventory', 'pipboy', 'character', 'barter', 'log', 'bark')]
    [string]$UiFixture,
    [ValidateSet('1.00', '1.08', '1.15', '1.22', '1.29')]
    [string]$Font101PreviewScale,
    [ValidateSet('bold', 'semibold')]
    [string]$Font101PreviewWeight,
    [string]$Tour,
    # The owner feedback shot (183): --dev-owner-shot-once takes the owner's
    # PNG and JSON at the first ready present (task 183's acceptance shots).
    # -OwnerShotDelay N waits N seconds of steady frames before the capture.
    [switch]$OwnerShot,
    [int]$OwnerShotDelay = 0,
    # The owner region mark (183 step 2): enter the mark mode after the view
    # settles and drop this many fixed rectangles, then save the clean and
    # marked PNGs with the regions JSON.
    [ValidateRange(0, 2)]
    [int]$OwnerMark = 0,
    [int]$OwnerMarkDelay = 0,
    # Seconds the game stays alive after the last snapshot, so a delayed
    # in-game capture (the owner shot) lands in steady frames.
    [int]$HoldSeconds = 0,
    [switch]$Classic,
    [switch]$Preferences,
    [string]$DevInput,
    # -SceneInput: a scripted-input script pumped by the main loop itself, with
    # the 3D scene up and no options screen on top (the scene begins the script
    # once its view shows). The script's actions are the -DevInput ones plus
    # "key <scancode>" (one pressed key, SDL_SCANCODE_* number) and "waitms".
    [string]$SceneInput,
    # Snapshots the script takes; the run waits for exactly this many (and the
    # script's PASS line). 5 matches the options-panel resolution script.
    [ValidateRange(1, 400)]
    [int]$DevInputShots = 5,
    # The script presses [ or ]: turn [3d] camera_rotation on in this run's
    # isolated config copy, so the keys reach the turn.
    [switch]$TurnKeys,
    # Turn [3d] fps_counter on in this run's isolated config copy, so the
    # presented frame (the watchdog capture) shows the readout.
    [switch]$FpsCounter,
    [ValidateSet('resolution', 'mode')]
    [string]$PreferencesDropdown,
    [switch]$DevPanel,
    [ValidateSet('Top', 'Bottom')]
    [string]$PreferencesPosition = 'Top',
    [switch]$NoPreview,
    [switch]$Parallel,
    [ValidateRange(0, 39999)]
    [int]$ViewTile,
    [ValidateRange(0, 39999)]
    [int]$HeroTile,
    [switch]$Still,
    [switch]$LiveFrame,
    [ValidateRange(0, 65536)]
    [int]$Ambient = -1,
    [switch]$Fly,
    [double]$FlyTurn = 0,
    [double]$FlyPitch = -1,
    [double]$FlyDistance = -1,
    [ValidateRange(-20, 20)]
    [double]$FlyLift = 0,
    # -Turn N: the 3D camera N quarter turns round ([3d] camera_rotation).
    [int]$Turn = 0,
    # Exact snapshot angle in degrees, bypassing player settings like -Turn.
    [ValidateRange(-360, 360)]
    [double]$TurnDegrees = 0,
    # -Solo R: only the things within R hexes of -ViewTile, no walls or roofs.
    [int]$Solo = -1,
    [string]$OutputDirectory,
    [string]$BuildExecutable,
    [string]$TestDirectory,
    [string]$GameRoot,
    [string]$SceneCache,
    [ValidateRange(1, 10)]
    [int]$DevLoadGameSlot = 0,
    [switch]$ConfiguredDisplay,
    [ValidateSet('english', 'russian')]
    [string]$Language,
    [ValidateSet('save', 'save-key', 'cancel', 'cancel-key', 'escape', 'defaults', 'verify')]
    [string]$PreferencesProbe,
    [switch]$ReuseTestConfig,
    [switch]$CaptureConsole,
    # Keep build/test-run/runs/<id> after the results are copied out.
    [switch]$KeepRunFolder,
    # -OpenDoors: every door on the map stands open (its last frame).
    [switch]$OpenDoors,
    # -Glow: keep the glow on in this background run (it is forced off by
    # default, so captures stay byte-stable).
    [switch]$Glow,
    # Implies -Glow. Five comparison levels can include values above the old limit.
    [ValidateRange(0, 8)]
    [double]$GlowStrength = -1,
    # Restore cycle.cc's rest-phase cycling colors before freezing the still view.
    [switch]$CyclePalette,
    # One frozen scene, six captures: off, then 0.5/1/2/4/8.
    [switch]$GlowLadder,
    [switch]$GlowPair,
    [ValidateCount(1, 8)]
    [ValidateRange(0, 8)]
    [double[]]$GlowLevels,
    [switch]$NoEffectEmission,
    [switch]$EffectLightCheck,
    [ValidateSet('laser', 'plasma-bolt', 'plasma-pistol', 'plasma-rifle', 'fire-explosion', 'plasma-explosion', 'emp-explosion', 'rocket-explosion', 'electric', 'flamethrower', 'rocket', 'sign40', 'sign41')]
    [string]$EffectFixture,
    [ValidateRange(1, 16777215)]
    [int]$MuzzleWeapon = 0,
    [ValidateSet(45,46,47)]
    [int]$MuzzleAnimation = 45,
    [ValidateRange(0,5)]
    [int]$MuzzleRotation = 1,
    [switch]$MuzzleGlowOff,
    [switch]$ClockLadder,
    [ValidateSet('left', 'right', 'up', 'down')]
    [string]$ScrollStep,
    [ValidateSet('left', 'right', 'up', 'down')]
    [string]$ScrollEdge,
    [switch]$FollowHero,
    [switch]$WalkProbe,
    [switch]$SkipScriptBlocking,
    [ValidateRange(0, 1000)]
    [int]$EffectFrame = -1,
    # -Shadows: keep sun and cloud shadows on in this background run (they are
    # forced off by default, so captures stay byte-stable).
    [switch]$Shadows,
    # -CloudShadows: clouds only, for an isolated cloud-shadow comparison.
    [switch]$CloudShadows,
    # -SunShadows: sun shadows only, for an isolated sun-shadow comparison.
    [switch]$SunShadows,
    # -CloudPhase: cloud drift phase in game minutes, independent of sun time.
    [double]$CloudPhase = -1,
    # -ShadowDebug:N: 1 the raw factor, 2 the face test (the sun hits the
    # face or it takes the flat shaded value), 3 the coverage (062
    # diagnostics); implies -Shadows.
    [int]$ShadowDebug = 0,
    # -NoSpriteSides: a turned camera sees the game's side of every sprite
    # (before 087).
    [switch]$NoSpriteSides,
    # -Hour H: set the game hour on the map (the sun snaps to it), so morning,
    # noon, sunset and night views come from one command.
    [ValidateRange(0, 24)]
    [double]$Hour = -1,
    # -SharpText: opt in to the experimental high-resolution interface text pass.
    [switch]$SharpText,
    # -Zoom D: the 3D zoom of the snapshots (1 is the game's own size).
    [ValidateRange(0.25, 4)]
    [double]$Zoom = -1,
    [ValidateRange(640, 7680)]
    [int]$ScreenWidth = 1920,
    [ValidateRange(480, 4320)]
    [int]$ScreenHeight = 1080,
    [ValidateRange(1.0, 3.0)]
    [double]$Scale = 2.0
)
$ErrorActionPreference = 'Stop'
if ($DevInput -and (-not $Preferences -or -not $ConfiguredDisplay -or -not $ReuseTestConfig -or -not $OutputDirectory -or -not $CaptureConsole)) {
    throw '-DevInput requires -Preferences, -ConfiguredDisplay, -ReuseTestConfig, -OutputDirectory and -CaptureConsole.'
}
if ($SceneInput -and (-not $OutputDirectory -or -not $CaptureConsole)) {
    throw '-SceneInput requires -OutputDirectory and -CaptureConsole.'
}
if ($SceneInput -and $DevInput) { throw '-SceneInput and -DevInput are two different script modes.' }
if ($SavePreviewFixture) {
    if (-not $OutputDirectory -or -not $CaptureConsole) {
        throw '-SavePreviewFixture requires -CaptureConsole and -OutputDirectory.'
    }
    if ($Probe -or $Tour -or $Preferences -or $DialogFixture -or $UiFixture -or $DevInput -or $SceneInput) {
        throw '-SavePreviewFixture needs its own background run.'
    }
}
if ($ExistingSavePreviewFixture) {
    if (-not $CaptureConsole -or -not $OutputDirectory) {
        throw '-ExistingSavePreviewFixture requires -CaptureConsole and -OutputDirectory.'
    }
    if ($Probe -or $Tour -or $Preferences -or $DialogFixture -or $UiFixture -or $DevInput -or $SceneInput -or $SavePreviewFixture) {
        throw '-ExistingSavePreviewFixture needs its own background run.'
    }
}
if ($TurnKeys -and -not $SceneInput) { throw '-TurnKeys requires -SceneInput.' }
if ($ClockLadder) {
    if (-not $OutputDirectory -or $Tour -or $Classic -or $Probe -or $Preferences -or $MuzzleWeapon -or $GlowLadder -or $GlowPair -or $GlowLevels -or $EffectFixture -or $DialogFixture -or $UiFixture) { throw '-ClockLadder requires its own scene run and -OutputDirectory.' }
    $Still=$true; $CaptureConsole=$true
}
if (([int][bool]$ScrollStep + [int][bool]$ScrollEdge + [int][bool]$FollowHero + [int][bool]$WalkProbe) -gt 1) { throw 'Choose one scroll or walk probe mode.' }
if ($ScrollStep -or $ScrollEdge -or $FollowHero -or $WalkProbe) {
    if (-not $OutputDirectory -or -not $PSBoundParameters.ContainsKey('ViewTile') -or $Tour -or $Probe -or $Preferences -or $MuzzleWeapon -or $GlowLadder -or $GlowPair -or $GlowLevels -or $EffectFixture -or $DialogFixture -or $UiFixture) {
        throw 'Scroll and walk probes require their own scene run, -ViewTile, and -OutputDirectory.'
    }
    if ($ScrollEdge -and -not $PSBoundParameters.ContainsKey('HeroTile')) { throw '-ScrollEdge requires -HeroTile near the map edge so hero-distance limiting does not stop the probe first.' }
    if ($FollowHero -and -not $PSBoundParameters.ContainsKey('HeroTile')) { throw '-FollowHero requires -HeroTile.' }
    if ($WalkProbe -and -not $PSBoundParameters.ContainsKey('HeroTile')) { throw '-WalkProbe requires -HeroTile.' }
    # Map scrolling is throttled by the live game clock; still mode freezes it
    # after the first step, so scroll probes must run with the normal tickers.
    $Still = $false
    $CaptureConsole = $true
}
if ($MuzzleWeapon -gt 0) {
    if ($EffectFixture -or $GlowPair -or $GlowLadder -or $GlowLevels -or $DialogFixture -or $UiFixture -or $Preferences -or $Tour -or $Classic -or $Probe -or $DevPanel -or $PreferencesProbe) {throw 'Muzzle batch needs its own ordinary scene run.'}
    if (-not $OutputDirectory) {throw '-MuzzleWeapon requires -OutputDirectory.'}
    $Still=$true; $CyclePalette=$true; $Glow=-not $MuzzleGlowOff; $CaptureConsole=$true
}
if (([int][bool]$GlowLadder + [int][bool]$GlowPair + [int][bool]$GlowLevels) -gt 1) { throw 'Choose one glow comparison mode.' }
if ($EffectLightCheck -and (-not $EffectFixture -or $EffectFixture -like 'sign*' -or $NoEffectEmission)) { throw '-EffectLightCheck requires an emitting projectile or explosion fixture.' }
if ($EffectFrame -ge 0 -and -not $EffectFixture) { throw '-EffectFrame requires -EffectFixture.' }
if ($SunShadows -and ($Shadows -or $CloudShadows -or $ShadowDebug)) { throw '-SunShadows cannot be combined with -Shadows, -CloudShadows, or -ShadowDebug.' }
if ($GlowLadder -or $GlowPair -or $GlowLevels -or $EffectFixture) {
    if ($Glow -or $GlowStrength -ge 0 -or $Preferences -or $Classic -or $Probe -or $Tour -or $DialogFixture -or $UiFixture -or $DevPanel -or $PreferencesProbe) {
        throw 'Glow comparisons and effect fixtures require an ordinary scene capture without other glow overrides or fixtures.'
    }
    $Still = $true
    $CyclePalette = $true
}
function Get-TestConfigValue([string]$Path, [string]$Section, [string]$Key) {
    $currentSection = ''
    $keyPattern = '^\s*' + [regex]::Escape($Key) + '\s*=\s*(.*?)\s*$'
    foreach ($line in [System.IO.File]::ReadAllLines($Path)) {
        if ($line -match '^\s*\[([^\]]+)\]') { $currentSection = $Matches[1] }
        elseif ($currentSection -eq $Section -and $line -match $keyPattern) { return $Matches[1] }
    }
    throw "Missing [$Section].$Key in $Path"
}
# Scene captures live beside the sidecar; classic framebuffer PNGs use the run
# folder's shots/game even though their sidecars are under shots/dev.
function Get-SnapshotImage($Snapshot, $Metadata) {
    $image = Join-Path $Snapshot.Directory $Metadata.image
    if (Test-Path -LiteralPath $image -PathType Leaf) { return $image }
    return Join-Path (Join-Path $runDirectory 'shots\game') $Metadata.image
}
# Once the results are copied out the run folder goes; without
# -OutputDirectory it holds them. A failed run's folder stays for a look.
function Complete-RunFolder {
    if ($OutputDirectory -and -not $KeepRunFolder) {
        Remove-Item -LiteralPath $runDirectory -Recurse -Force
    } else {
        Write-Output "RUN FOLDER: $runDirectory"
    }
}
if ($DevPanel) {
    if ($Preferences -or $PreferencesProbe -or $DialogFixture -or $UiFixture -or $Probe -or $Tour) {
        throw '-DevPanel cannot be combined with other UI fixtures, Probe, or Tour.'
    }
    # Static 3D captures redraw the world without UI. Use the framebuffer so
    # the developer panel is present in the saved picture.
    $Classic = $true
}
$Root = Split-Path -Parent $PSScriptRoot
$GameDir = if ($GameRoot) {
    if ([System.IO.Path]::IsPathRooted($GameRoot)) { [System.IO.Path]::GetFullPath($GameRoot) }
    else { [System.IO.Path]::GetFullPath((Join-Path $Root $GameRoot)) }
} else { Join-Path $Root 'game' }
if (-not $GameRoot -and -not (Test-Path -LiteralPath $GameDir)) {
    # An agent's worktree has no game: use the main checkout's, with this build.
    $main = (git -C $Root worktree list --porcelain | Select-Object -First 1) -replace '^worktree ', ''
    $GameDir = Join-Path $main 'game'
}
if (-not (Test-Path -LiteralPath $GameDir -PathType Container)) { throw "No game root at $GameDir" }
$built = if ($BuildExecutable) { [System.IO.Path]::GetFullPath((Join-Path $Root $BuildExecutable)) } else { Join-Path $Root 'build\fallout2-ce.exe' }
if (-not (Test-Path $built)) { throw "No build at $built; run tools/configure-build.ps1 first." }
# The exe's own folder holds the config: build it fresh from the fixed one, so
# a saved setting can never leak into the next run.
$fixedConfig = Join-Path $PSScriptRoot 'test-config\fallout2.cfg'
if (-not (Test-Path -LiteralPath $fixedConfig)) { throw "Missing the fixed test config: $fixedConfig" }
$exeDirectory = if ($TestDirectory) { [System.IO.Path]::GetFullPath((Join-Path $Root $TestDirectory)) } else { Join-Path $Root 'build\test-run' }
New-Item -ItemType Directory -Path $exeDirectory -Force | Out-Null
$exe = Join-Path $exeDirectory 'fallout2-ce-test.exe'
# One test game at a time per worktree (they share this exe and its config);
# other worktrees run alongside. Wait here for the slot instead of failing, so
# an agent spends one call on it, not a polling loop. The mutex lines the
# waiters up; it is let go once the test game has exited, or by the trap on
# any error, so a long-lived PowerShell host never keeps it.
$slotKey = [System.BitConverter]::ToString([System.Security.Cryptography.SHA256]::Create().ComputeHash(
    [System.Text.Encoding]::UTF8.GetBytes($exe.ToLowerInvariant()))).Replace('-', '').Substring(0, 16)
$slot = New-Object System.Threading.Mutex($false, "Global\fallout2-ce-test-slot-$slotKey")
$slotHeld = $false
function Release-TestSlot {
    if ($script:slotHeld) {
        $script:slotHeld = $false
        $slot.ReleaseMutex()
        $slot.Dispose()
    }
}
trap { Release-TestSlot; break }
try { $slotHeld = $slot.WaitOne([TimeSpan]::FromMinutes($WaitMinutes)) }
catch [System.Threading.AbandonedMutexException] { $slotHeld = $true }
if (-not $slotHeld) { throw "The test slot stayed busy for $WaitMinutes minutes." }
$slotDeadline = (Get-Date).AddMinutes($WaitMinutes)
# A game this worktree started outside the slot (an older script) still holds
# the exe; other worktrees' test games are left alone.
while (Get-Process fallout2-ce-test -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $exe }) {
    if ((Get-Date) -gt $slotDeadline) { throw "$exe kept running for $WaitMinutes minutes." }
    Start-Sleep -Seconds 2
}
$configPath = Join-Path $exeDirectory 'fallout2.cfg'
if ($PreferencesProbe) {
    if (-not $OutputDirectory -or -not $CaptureConsole) { throw '-PreferencesProbe requires -CaptureConsole and -OutputDirectory.' }
    if ($PreferencesProbe -eq 'verify' -and -not $ReuseTestConfig) { throw 'Use -ReuseTestConfig with the read-only verify probe.' }
} elseif ($ReuseTestConfig) {
    if (-not $ConfiguredDisplay) { throw '-ReuseTestConfig requires -PreferencesProbe or -ConfiguredDisplay.' }
}
if ($DialogFixture) {
    if ($Map -ne 'kladwtwn') { throw '-DialogFixture requires the resource-verified Klamath map kladwtwn.' }
    if ($Preferences -or $Classic -or $Probe -or $Tour -or $Still -or $NoPreview -or $PreferencesProbe) {
        throw '-DialogFixture cannot be combined with Preferences, Classic, Probe, Tour, Still, NoPreview, or PreferencesProbe.'
    }
    if (-not $CaptureConsole -or -not $OutputDirectory) {
        throw '-DialogFixture requires -CaptureConsole and -OutputDirectory for fixture diagnostics.'
    }
}
if ($UiFixture) {
    if ($DialogFixture -or $DialogPageInspect -or $DialogCursorCheck -or $Preferences -or $PreferencesProbe -or
        $Classic -or $Probe -or $Tour -or $Still -or $NoPreview) {
        throw '-UiFixture cannot be combined with DialogFixture, Preferences, Classic, Probe, Tour, Still, NoPreview, or PreferencesProbe.'
    }
    if ($UiFixture -eq 'barter' -and $Map -ne 'kladwtwn') {
        throw '-UiFixture barter requires the resource-verified Klamath map kladwtwn.'
    }
    if (-not $CaptureConsole -or -not $OutputDirectory) {
        throw '-UiFixture requires -CaptureConsole and -OutputDirectory for fixture diagnostics.'
    }
}
if ($Font101PreviewScale) {
    if ($UiFixture -ne 'character') { throw '-Font101PreviewScale requires -UiFixture character.' }
    if (-not $SharpText) { throw '-Font101PreviewScale requires -SharpText.' }
}
if ($Font101PreviewWeight) {
    if ($UiFixture -ne 'character') { throw '-Font101PreviewWeight requires -UiFixture character.' }
    if (-not $SharpText) { throw '-Font101PreviewWeight requires -SharpText.' }
    if (-not $Font101PreviewScale) { throw '-Font101PreviewWeight requires -Font101PreviewScale.' }
}
if ($DialogPageInspect) {
    if (-not $DialogFixture) { throw '-DialogPageInspect requires -DialogFixture.' }
    if ($SharpText) { throw '-DialogPageInspect must inspect the original pixel text with sharp text off.' }
    if ($Language -ne 'english') { throw '-DialogPageInspect requires -Language english so the recorded reply text is readable.' }
}
if ($DialogCursorCheck) {
    if (-not $DialogFixture) { throw '-DialogCursorCheck requires -DialogFixture.' }
    if (-not $SharpText) { throw '-DialogCursorCheck requires -SharpText.' }
    if ($DialogPageInspect) { throw '-DialogCursorCheck cannot be combined with -DialogPageInspect.' }
    if ($Language -ne 'english') { throw '-DialogCursorCheck requires -Language english for readable diagnostics.' }
}
if ($DialogLayoutCheck) {
    if (-not $DialogFixture) { throw '-DialogLayoutCheck requires -DialogFixture.' }
    if ($DialogPageInspect -or $DialogCursorCheck) { throw '-DialogLayoutCheck cannot be combined with other dialog checks.' }
}
if ($ReuseTestConfig -or $ConfiguredDisplay) {
    if (-not (Test-Path -LiteralPath $configPath -PathType Leaf)) { throw "No isolated config to reuse: $configPath" }
} else {
    Copy-Item $fixedConfig $configPath -Force
}
if ($Language) {
    $configLines = [System.Collections.Generic.List[string]]::new()
    $inSystemSection = $false
    $languageFound = $false
    foreach ($line in [System.IO.File]::ReadAllLines($configPath)) {
        if ($line -match '^\s*\[') { $inSystemSection = $line.Trim() -eq '[system]' }
        if ($inSystemSection -and $line -match '^\s*language=') {
            $configLines.Add("language=$Language")
            $languageFound = $true
        } else {
            $configLines.Add($line)
        }
    }
    if (-not $languageFound) { throw 'Could not find [system].language in the isolated test config.' }
    [System.IO.File]::WriteAllLines($configPath, $configLines, [System.Text.UTF8Encoding]::new($false))
}
if ($TurnKeys) {
    $configLines = [System.Collections.Generic.List[string]]::new()
    $in3dSection = $false
    $rotationFound = $false
    foreach ($line in [System.IO.File]::ReadAllLines($configPath)) {
        if ($line -match '^\s*\[') { $in3dSection = $line.Trim() -eq '[3d]' }
        if ($in3dSection -and $line -match '^\s*camera_rotation=') {
            $configLines.Add('camera_rotation=1')
            $rotationFound = $true
        } else {
            $configLines.Add($line)
        }
    }
    if (-not $rotationFound) { throw 'Could not find [3d].camera_rotation in the isolated test config.' }
    [System.IO.File]::WriteAllLines($configPath, $configLines, [System.Text.UTF8Encoding]::new($false))
}
if ($FpsCounter) {
    $configLines = [System.Collections.Generic.List[string]]::new()
    $in3dSection = $false
    $counterFound = $false
    foreach ($line in [System.IO.File]::ReadAllLines($configPath)) {
        if ($line -match '^\s*\[') { $in3dSection = $line.Trim() -eq '[3d]' }
        if ($in3dSection -and $line -match '^\s*fps_counter=') {
            $configLines.Add('fps_counter=1')
            $counterFound = $true
        } else {
            $configLines.Add($line)
        }
    }
    if (-not $counterFound) { throw 'Could not find [3d].fps_counter in the isolated test config.' }
    [System.IO.File]::WriteAllLines($configPath, $configLines, [System.Text.UTF8Encoding]::new($false))
}
$configHashBefore = (Get-FileHash -LiteralPath $configPath -Algorithm SHA256).Hash
$initialZoom = Get-TestConfigValue $configPath '3d' 'zoom'
$initialEnabled = Get-TestConfigValue $configPath '3d' 'enabled'
$initialDifficulty = Get-TestConfigValue $configPath 'preferences' 'combat_difficulty'
if ($DevInput -and $Preferences -and $ConfiguredDisplay) {
    $initialDisplayWidth = Get-TestConfigValue $configPath 'screen' 'resolution_x'
    $initialDisplayHeight = Get-TestConfigValue $configPath 'screen' 'resolution_y'
    $initialDisplayScale = Get-TestConfigValue $configPath 'screen' 'scale_factor'
}
$logicalWidth = [int][Math]::Round($ScreenWidth / $Scale, [MidpointRounding]::AwayFromZero)
$logicalHeight = [int][Math]::Round($ScreenHeight / $Scale, [MidpointRounding]::AwayFromZero)
if ($logicalWidth -lt 640 -or $logicalHeight -lt 480) {
    throw 'The requested scale must leave at least a 640x480 logical framebuffer.'
}
if (-not (Test-Path -LiteralPath $exe) -or
    (Get-FileHash -LiteralPath $built).Hash -ne (Get-FileHash -LiteralPath $exe).Hash) {
    Copy-Item $built $exe -Force
}
$shots = 1
$runId = [guid]::NewGuid().ToString('N')
# This run's own output root: the exe writes everything under it. A short
# name, as the game's own paths stay within 260 characters.
$runDirectory = Join-Path $exeDirectory "runs\$($runId.Substring(0, 12))"
New-Item -ItemType Directory -Path $runDirectory -Force | Out-Null
if ($DevLoadGameSlot -gt 0) {
    # The run's redirected state starts empty, so a save load would wait on
    # the load screen forever: seed this run's SAVEGAME from the game root.
    $saveSource = Join-Path $GameDir 'data\SAVEGAME'
    if (Test-Path -LiteralPath $saveSource) {
        $saveDestination = Join-Path $runDirectory 'data\SAVEGAME'
        New-Item -ItemType Directory -Path $saveDestination -Force | Out-Null
        Copy-Item -Path (Join-Path $saveSource '*') -Destination $saveDestination -Recurse -Force
    }
}
$arguments = @('--dev-tools')
$arguments += '--dev-watchdog'
if ($SavePreviewFixture) { $arguments += '--dev-save-preview-fixture' }
if ($ExistingSavePreviewFixture) { $arguments += '--dev-existing-save-preview-fixture' }
if ($DevInput) { $arguments += "--dev-input=$([System.IO.Path]::GetFullPath($DevInput))" }
if ($SceneInput) { $arguments += "--dev-scene-input=$([System.IO.Path]::GetFullPath($SceneInput))" }
if ($TurnKeys) { $arguments += '--dev-camera-turn-keys' }
if ($OwnerShot) {
    if ($OwnerShotDelay -gt 0) { $arguments += "--dev-owner-shot-once=$OwnerShotDelay" }
    else { $arguments += '--dev-owner-shot-once' }
}
if ($OwnerMark -eq 1) {
    if ($OwnerMarkDelay -gt 0) { $arguments += "--dev-owner-mark-once=$OwnerMarkDelay" }
    else { $arguments += '--dev-owner-mark-once' }
}
if ($OwnerMark -eq 2) {
    if ($OwnerMarkDelay -gt 0) { $arguments += "--dev-owner-mark-twice=$OwnerMarkDelay" }
    else { $arguments += '--dev-owner-mark-twice' }
}
if ($MuzzleWeapon -gt 0) {$arguments += @("--dev-muzzle=$MuzzleWeapon", "--dev-muzzle-animation=$MuzzleAnimation", "--dev-muzzle-rotation=$MuzzleRotation")}
if ($ClockLadder) { $arguments += '--dev-clock-ladder' }
    if ($ScrollStep) { $arguments += "--dev-scroll-step=$ScrollStep" }
if ($ScrollEdge) { $arguments += "--dev-scroll-edge=$ScrollEdge" }
if ($FollowHero) { $arguments += '--dev-follow-hero' }
    if ($WalkProbe) { $arguments += '--dev-walk-probe' }
if ($SkipScriptBlocking) { $arguments += '--dev-skip-script-blocking' }
if ($NoEffectEmission) { $arguments += '--dev-no-effect-emission' }
if ($EffectLightCheck) { $arguments += '--dev-effect-light-check' }
if ($SharpText) { $arguments += '--dev-sharp-text=1' } else { $arguments += '--dev-sharp-text=0' }
if ($UiFixture) { $arguments += "--dev-ui-fixture=$UiFixture" }
if ($Font101PreviewScale) { $arguments += "--dev-font101-size-preview=$Font101PreviewScale" }
if ($Font101PreviewWeight) { $arguments += "--dev-font101-weight-preview=$Font101PreviewWeight" }
if ($UiFixture -eq 'log') { $arguments += '--dev-classic' }
if ($DialogFixture) { $arguments += '--dev-dialog-fixture' }
if ($DevPanel) { $arguments += '--dev-panel-snapshot' }
if ($DialogPageInspect) { $arguments += '--dev-dialog-page-inspect' }
if ($DialogCursorCheck) { $arguments += '--dev-dialog-cursor-check' }
if ($DialogLayoutCheck) { $arguments += '--dev-dialog-layout-check' }
if ($PreferencesProbe) {
    $arguments += @('--dev-new-game', "--dev-map=$Map", "--dev-elevation=$Elevation", "--dev-preferences-probe=$PreferencesProbe")
} elseif ($ConfiguredDisplay) {
    if ($PreferencesProbe) { throw '-ConfiguredDisplay cannot be combined with -PreferencesProbe.' }
    if ($DevLoadGameSlot -gt 0) { $arguments += "--dev-load-game=$DevLoadGameSlot" }
    else { $arguments += @('--dev-new-game', "--dev-map=$Map", "--dev-elevation=$Elevation") }
    $arguments += "--dev-snapshot-run=$runId"
} else {
    $scaleText = $Scale.ToString('0.0#', [Globalization.CultureInfo]::InvariantCulture)
    $scaleArgument = if ([Math]::Abs($Scale - [Math]::Round($Scale)) -lt 0.0001) { "--dev-scale=$([int][Math]::Round($Scale))" } else { "--dev-scale-factor=$scaleText" }
    $arguments += @("--dev-screen=${ScreenWidth}x${ScreenHeight}", $scaleArgument)
    if ($DevLoadGameSlot -gt 0) { $arguments += "--dev-load-game=$DevLoadGameSlot" }
    else { $arguments += @('--dev-new-game', "--dev-map=$Map", "--dev-elevation=$Elevation") }
    $arguments += "--dev-snapshot-run=$runId"
}
if ($Glow) { $arguments += '--dev-glow' }
if ($GlowStrength -ge 0) { $arguments += "--dev-glow-strength=$($GlowStrength.ToString([System.Globalization.CultureInfo]::InvariantCulture))" }
if ($CyclePalette) { $arguments += '--dev-cycle-palette' }
if ($GlowLadder) { $arguments += '--dev-glow-ladder' }
if ($GlowPair) { $arguments += '--dev-glow-pair' }
if ($GlowLevels) { $arguments += '--dev-glow-levels=' + (($GlowLevels | ForEach-Object { $_.ToString('0.###', [Globalization.CultureInfo]::InvariantCulture) }) -join ',') }
if ($EffectFixture) { $arguments += "--dev-effect=$($EffectFixture.ToLowerInvariant())" }
if ($EffectFrame -ge 0) { $arguments += "--dev-effect-frame=$EffectFrame" }
if ($Shadows) { $arguments += '--dev-shadows' }
if ($CloudShadows) { $arguments += '--dev-cloud-shadows' }
if ($SunShadows) { $arguments += '--dev-sun-shadows' }
if ($CloudPhase -ge 0) { $arguments += "--dev-cloud-phase=$($CloudPhase.ToString([System.Globalization.CultureInfo]::InvariantCulture))" }
if ($ShadowDebug) { $arguments += "--dev-shadow-debug=$ShadowDebug" }
if ($NoSpriteSides) { $arguments += '--dev-no-sprite-sides' }
if ($OpenDoors) { $arguments += '--dev-open-doors' }
if ($Hour -ge 0) { $arguments += "--dev-hour=$Hour" }
if ($Zoom -gt 0) { $arguments += "--dev-zoom=$Zoom" }
if ($PreferencesProbe) {
    if ($Preferences -or $Classic -or $Probe -or $Tour -or $Still) { throw '-PreferencesProbe cannot be combined with another capture mode.' }
} elseif ($Preferences) {
    if ($Probe -or $Tour -or $Still) { throw '-Preferences cannot be combined with -Probe, -Tour, or -Still.' }
    if ($Classic) { $arguments += '--dev-classic' }
    $arguments += '--dev-preferences-snapshot'
    if ($PreferencesDropdown) { $arguments += "--dev-preferences-dropdown=$PreferencesDropdown" }
    if ($PreferencesPosition -eq 'Bottom') { $arguments += '--dev-preferences-bottom' }
} elseif ($Classic) { $arguments += '--dev-classic' }
elseif (-not $NoPreview) { $arguments += '--dev-hero-preview' }
if ($PSBoundParameters.ContainsKey('ViewTile')) { $arguments += "--dev-view-tile=$ViewTile" }
if ($PSBoundParameters.ContainsKey('HeroTile')) { $arguments += "--dev-hero-tile=$HeroTile" }
# -Parallel: no perspective, so the scene lies on the game's 2D pixel for pixel.
if ($Parallel) { $arguments += '--dev-parallel' }
# -Ambient: the map's ambient light held at this level (16384 is the game's
# darkest night, 65536 full day).
if ($Ambient -ge 0) { $arguments += "--dev-ambient=$Ambient" }
# -Fly: the snapshot is taken from the fly camera's start (Ctrl+F12 in the game).
if ($Fly) { $arguments += '--dev-fly' }
# -FlyTurn D, -FlyPitch D, -FlyDistance U: from there the camera circles the
# point it looks at, D degrees round, D degrees above the floor, U units from
# it (each implies -Fly).
if ($FlyTurn -ne 0) { $arguments += "--dev-fly-turn=$FlyTurn" }
if ($FlyPitch -gt 0) { $arguments += "--dev-fly-pitch=$FlyPitch" }
if ($FlyDistance -gt 0) { $arguments += "--dev-fly-distance=$FlyDistance" }
if ($FlyLift -ne 0) { $arguments += "--dev-fly-lift=$($FlyLift.ToString([System.Globalization.CultureInfo]::InvariantCulture))" }
if ($Turn -ne 0) { $arguments += "--dev-camera-turn=$Turn" }
if ($TurnDegrees -ne 0) { $arguments += "--dev-camera-degrees=$($TurnDegrees.ToString([System.Globalization.CultureInfo]::InvariantCulture))" }
if ($Solo -ge 0 -and $PSBoundParameters.ContainsKey('ViewTile')) { $arguments += "--dev-solo=$ViewTile,$Solo" }
if ($Still) {
    if ($Probe) { throw '-Still cannot be combined with the interactive -Probe.' }
    $arguments += '--dev-still-snapshot'
}
if ($LiveFrame) {
    if ($Still -or $Preferences -or $PreferencesProbe -or $Probe -or $Tour -or $DialogFixture -or $UiFixture) {
        throw '-LiveFrame cannot be combined with a still, preference, probe, tour, dialog, or UI fixture mode.'
    }
    $arguments += '--dev-live-snapshot-once'
}
if ($DevLoadGameSlot -gt 0 -and -not $LiveFrame) {
    throw '-DevLoadGameSlot requires -LiveFrame.'
}
if ($SavePreviewFixture) {
    $shots = 2
} elseif ($ExistingSavePreviewFixture) {
    $shots = 1
} elseif ($DevInput -or $SceneInput) {
    $shots = $DevInputShots
} elseif ($LiveFrame) {
    $shots = 1
} elseif ($PreferencesProbe) {
    $shots = 0
} elseif ($Preferences) {
    $shots = 1
} elseif ($Probe) {
    if ($Tour) { throw '-Tour cannot be combined with -Probe.' }
    $shots = 2
    $arguments += '--dev-pick-probe'
} elseif ($ScrollStep -or $ScrollEdge -or $FollowHero) {
    $shots = 1
    $arguments += '--dev-snapshot-once'
} elseif ($Tour -or $Classic) {
    # Without the scene the one snapshot comes from a tour with no stops.
    $shots = 1 + @("$Tour".Split(',') | Where-Object { $_.Trim() }).Count
    $arguments += "--dev-map-tour=$Tour"
} else {
    if ($UiFixture) { $shots = if ($UiFixture -eq 'bark') { 2 } else { 1 } }
    elseif ($DialogPageInspect) { $shots = 0 }
    elseif ($DialogCursorCheck) { $shots = 1 }
    elseif ($DialogFixture) { $shots = 1 }
    else { $arguments += '--dev-snapshot-once' }
}
if ($GlowLadder) { $shots = 6 }
if ($GlowPair) { $shots = 2 }
if ($GlowLevels) { $shots = 1 + $GlowLevels.Count }
if ($ClockLadder) { $shots = 4 }

$started = Get-Date
$previousBackground = $env:F2CE_DEV_BACKGROUND
$env:F2CE_DEV_BACKGROUND = '1'
# No sound device at all: a background run must not hiss or play.
$previousAudio = $env:SDL_AUDIODRIVER
$env:SDL_AUDIODRIVER = 'dummy'
$previousSceneCache = $env:F2CE_SCENE_CACHE
$previousOutput = $env:F2CE_DEV_OUT
$env:F2CE_DEV_OUT = $runDirectory
$worktreeCache = Join-Path $Root 'build\scene-cache'
if ($SceneCache) {
    if (-not (Test-Path -LiteralPath $SceneCache -PathType Container)) { throw "No scene cache at $SceneCache" }
    $env:F2CE_SCENE_CACHE = (Resolve-Path -LiteralPath $SceneCache).Path
} elseif (Test-Path -LiteralPath $worktreeCache -PathType Container) { $env:F2CE_SCENE_CACHE = $worktreeCache }
$process = $null
$probeHarnessStoppedProcess = $false
$snapshots = @()
$dialogFixtureFailed = $false
$watchdogExpired = $false
$watchdogReason = ''
$watchdogLastProgress = Get-Date
$watchdogLastLine = ''
$watchdogProgressObserved = $false
$watchdogStdout = if ($CaptureConsole) { Join-Path $OutputDirectory 'process.stdout.log' } else { Join-Path $runDirectory 'process.stdout.log' }
$watchdogStderr = if ($CaptureConsole) { Join-Path $OutputDirectory 'process.stderr.log' } else { Join-Path $runDirectory 'process.stderr.log' }
$shotPath = Join-Path $runDirectory 'watchdog.png'
try {
    $startParameters = @{
        FilePath = $exe
        WorkingDirectory = $GameDir
        WindowStyle = 'Hidden'
        PassThru = $true
        ArgumentList = $arguments
    }
    if ($CaptureConsole -and -not $OutputDirectory) { throw '-CaptureConsole requires -OutputDirectory.' }
    if ($OutputDirectory) { New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null }
    $startParameters.RedirectStandardOutput = $watchdogStdout
    $startParameters.RedirectStandardError = $watchdogStderr
    $process = Start-Process @startParameters
    $deadline = $started.AddSeconds($TimeoutSeconds)
    $probeCompletedBeforeDeadline = $false
    if ($PreferencesProbe) {
        $probeStdoutPath = Join-Path $OutputDirectory 'process.stdout.log'
        while ((Get-Date) -lt $deadline -and -not $process.HasExited) {
            if (Test-Path -LiteralPath $watchdogStdout) {
                $lines = Get-Content -LiteralPath $watchdogStdout -Tail 40 -ErrorAction SilentlyContinue
                $progress = @($lines | Where-Object { $_ -match '(^| )(STEP|START|PASS|FAIL|RESULT|DEV_INPUT pushed|DEV_INPUT snap|pending map id|waiting for map id)' } | Select-Object -Last 1)
                if ($progress.Count -gt 0 -and $progress[0] -ne $watchdogLastLine) {
                    $watchdogLastLine = [string]$progress[0]
                    $watchdogLastProgress = Get-Date
                    $watchdogProgressObserved = $true
                }
            }
            if ($watchdogProgressObserved -and ((Get-Date) - $watchdogLastProgress).TotalSeconds -ge $StepTimeoutSeconds) {
                $watchdogExpired = $true
                $watchdogReason = "step timeout after $StepTimeoutSeconds s without progress"
                break
            }
            if ((Test-Path -LiteralPath $probeStdoutPath) -and
                (Select-String -LiteralPath $probeStdoutPath -Pattern 'PREFERENCES_PROBE finished; requesting isolated helper exit' -Quiet)) {
                $probeCompletedBeforeDeadline = $true
                break
            }
            Start-Sleep -Milliseconds 250
        }
    } else {
        while ((Get-Date) -lt $deadline -and -not $process.HasExited) {
            if (Test-Path -LiteralPath $watchdogStdout) {
                $lines = Get-Content -LiteralPath $watchdogStdout -Tail 40 -ErrorAction SilentlyContinue
                $progress = @($lines | Where-Object { $_ -match '(^| )(STEP|START|PASS|FAIL|RESULT|DEV_INPUT pushed|DEV_INPUT snap|pending map id|waiting for map id)' } | Select-Object -Last 1)
                if ($progress.Count -gt 0 -and $progress[0] -ne $watchdogLastLine) {
                    $watchdogLastLine = [string]$progress[0]
                    $watchdogLastProgress = Get-Date
                    $watchdogProgressObserved = $true
                }
            }
            if ($watchdogProgressObserved -and ((Get-Date) - $watchdogLastProgress).TotalSeconds -ge $StepTimeoutSeconds) {
                $watchdogExpired = $true
                $watchdogReason = "step timeout after $StepTimeoutSeconds s without progress"
                break
            }
            # The exe writes into shots\dev\<date>\; the root scan also catches a
            # writer that could not make the folder. Both are filtered by the run
            # id below.
            $snapshots = @(
                @(Get-ChildItem $runDirectory -Filter 'devshot-*.json' -File -ErrorAction SilentlyContinue) +
                @(Get-ChildItem (Join-Path $runDirectory 'shots\dev') -Recurse -Filter 'devshot-*.json' -File -ErrorAction SilentlyContinue)
            ) | Where-Object {
                if ($_.LastWriteTime -le $started) { return $false }
                try { (Get-Content -Raw -LiteralPath $_.FullName | ConvertFrom-Json).snapshot_run -eq $runId }
                catch { $false } # The writer may still be completing this file.
            }
            $log = Join-Path $runDirectory 'logs\devprobe.log'
            $probed = -not $Probe -or ((Test-Path $log) -and (Select-String -Path $log -Pattern 'probe done' -Quiet))
            if ($DevInput -or $SceneInput) {
                $inputLog = Join-Path $OutputDirectory 'process.stdout.log'
                if ((Test-Path -LiteralPath $inputLog) -and (Select-String -LiteralPath $inputLog -Pattern '^DEV_INPUT FAIL' -Quiet)) { break }
                if ($snapshots.Count -ge $shots -and (Test-Path -LiteralPath $inputLog) -and (Select-String -LiteralPath $inputLog -Pattern '^DEV_INPUT PASS script complete' -Quiet)) { break }
            } elseif ($UiFixture) {
                $fixtureLog = Join-Path $OutputDirectory 'process.stdout.log'
                if ((Test-Path -LiteralPath $fixtureLog) -and
                    ((Select-String -LiteralPath $fixtureLog -Pattern '^UI_FIXTURE FAIL ' -Quiet) -or
                        ($UiFixture -eq 'barter' -and (Select-String -LiteralPath $fixtureLog -Pattern '^DIALOG_FIXTURE FAIL ' -Quiet)))) {
                    $dialogFixtureFailed = $true
                    break
                }
                if ($snapshots.Count -ge $shots -and (Test-Path -LiteralPath $fixtureLog) -and
                    (Select-String -LiteralPath $fixtureLog -Pattern ('^UI_FIXTURE PASS screen=' + [regex]::Escape($UiFixture) + ' ') -Quiet)) { break }
            } elseif ($SavePreviewFixture) {
                $fixtureLog = Join-Path $OutputDirectory 'process.stdout.log'
                if ((Test-Path -LiteralPath $fixtureLog) -and (Select-String -LiteralPath $fixtureLog -Pattern '^SAVE_PREVIEW_FIXTURE FAIL ' -Quiet)) { break }
                if ($snapshots.Count -ge 1 -and (Test-Path -LiteralPath $fixtureLog) -and
                    (Select-String -LiteralPath $fixtureLog -Pattern '^SAVE_PREVIEW_FIXTURE PASS ' -Quiet)) { break }
            } elseif ($DialogPageInspect) {
                $fixtureLog = Join-Path $OutputDirectory 'process.stdout.log'
                if ((Test-Path -LiteralPath $fixtureLog) -and
                    (Select-String -LiteralPath $fixtureLog -Pattern '^DIALOG_PAGE_INSPECT PASS ' -Quiet)) { break }
                if ((Test-Path -LiteralPath $fixtureLog) -and
                    (Select-String -LiteralPath $fixtureLog -Pattern '^DIALOG_FIXTURE FAIL ' -Quiet)) { $dialogFixtureFailed = $true; break }
            } elseif ($DialogCursorCheck) {
                $fixtureLog = Join-Path $OutputDirectory 'process.stdout.log'
                if ((Test-Path -LiteralPath $fixtureLog) -and
                    (Select-String -LiteralPath $fixtureLog -Pattern '^DIALOG_CURSOR_CHECK FAIL ' -Quiet)) { $dialogFixtureFailed = $true; break }
                if ($snapshots.Count -ge $shots -and (Test-Path -LiteralPath $fixtureLog) -and
                    (Select-String -LiteralPath $fixtureLog -Pattern '^DIALOG_CURSOR_CHECK PASS ' -Quiet)) { break }
            } elseif ($DialogLayoutCheck) {
                $fixtureLog = Join-Path $OutputDirectory 'process.stdout.log'
                if ((Test-Path -LiteralPath $fixtureLog) -and
                    (Select-String -LiteralPath $fixtureLog -Pattern '^DIALOG_LAYOUT_CHECK FAIL ' -Quiet)) { $dialogFixtureFailed = $true; break }
                if ($snapshots.Count -ge $shots -and (Test-Path -LiteralPath $fixtureLog) -and
                    (Select-String -LiteralPath $fixtureLog -Pattern '^DIALOG_LAYOUT_CHECK PASS ' -Quiet) -and
                    (Select-String -LiteralPath $fixtureLog -Pattern '^DIALOG_FIXTURE PASS ' -Quiet)) { break }
            } elseif ($DialogFixture) {
                $fixtureLog = Join-Path $OutputDirectory 'process.stdout.log'
                if ((Test-Path -LiteralPath $fixtureLog) -and
                    (Select-String -LiteralPath $fixtureLog -Pattern '^DIALOG_FIXTURE FAIL ' -Quiet)) {
                    $dialogFixtureFailed = $true
                    break
                }
                if ($snapshots.Count -ge $shots -and (Test-Path -LiteralPath $fixtureLog) -and
                    (Select-String -LiteralPath $fixtureLog -Pattern '^DIALOG_FIXTURE PASS ' -Quiet)) { break }
            } elseif ($MuzzleWeapon -gt 0) {
                $muzzleLog086=Join-Path $OutputDirectory 'process.stdout.log'
                if (Test-Path -LiteralPath $muzzleLog086) {
                    $muzzleText086=Get-Content -Raw -LiteralPath $muzzleLog086
                    if ($muzzleText086 -match 'MUZZLE_FIXTURE FAIL') {break}
                    if ($muzzleText086 -match 'MUZZLE_FIXTURE PASS frames=(\d+) captures=(\d+)') {
                        if ($snapshots.Count -ge [int]$Matches[2]) {break}
                    }
                }
            } elseif ($ClockLadder) {
                $clockLog = Join-Path $OutputDirectory 'process.stdout.log'
                if ($snapshots.Count -ge $shots -and (Test-Path -LiteralPath $clockLog) -and (Select-String -LiteralPath $clockLog -Pattern '^CLOCK_LADDER PASS' -Quiet)) { break }
            } elseif ($ScrollStep -or $ScrollEdge -or $FollowHero -or $WalkProbe) {
                $scrollLog = Join-Path $OutputDirectory 'process.stdout.log'
                if ((Test-Path -LiteralPath $scrollLog) -and
                    (Select-String -LiteralPath $scrollLog -Pattern '^(SCROLL_STEP|SCROLL_EDGE|FOLLOW_HERO) FAIL ' -Quiet)) { break }
                $scrollProbePassed = (Test-Path -LiteralPath $scrollLog) -and
                    (Select-String -LiteralPath $scrollLog -Pattern '^(SCROLL_STEP|SCROLL_EDGE|FOLLOW_HERO) PASS ' -Quiet)
                $walkProbeFinished = $WalkProbe -and (Test-Path -LiteralPath $scrollLog) -and
                    (Select-String -LiteralPath $scrollLog -Pattern '^WALK_PROBE RESULT ' -Quiet)
                if ($snapshots.Count -ge $shots -and ($scrollProbePassed -or $walkProbeFinished)) { break }
            } elseif ($snapshots.Count -ge $shots -and $probed) { break }
            Start-Sleep -Milliseconds 500
        }
    }
    if (-not $process.HasExited -and -not $watchdogExpired -and (Get-Date) -ge $deadline) {
        $watchdogExpired = $true
        $watchdogReason = "whole-run timeout after $TimeoutSeconds s"
    }
    if ($watchdogExpired -and $null -ne $process -and -not $process.HasExited) {
        $stateLines = if (Test-Path -LiteralPath $watchdogStdout) { @(Get-Content -LiteralPath $watchdogStdout -Tail 80 -ErrorAction SilentlyContinue) } else { @() }
        $lastStep = if ($watchdogLastLine) { $watchdogLastLine } else { '<no progress marker>' }
        $diag = "WATCHDOG STOP reason=$watchdogReason`nlast_step=$lastStep`nlast_game_state=$(@($stateLines | Where-Object { $_ -match 'pending map id|waiting for map id|WALK_PROBE|DEV_INPUT|TOUR|COMBAT|DEV_WATCHDOG state' } | Select-Object -Last 1))"
        $diagPath = Join-Path $runDirectory 'watchdog.txt'
        Set-Content -LiteralPath $diagPath -Value $diag -Encoding utf8
        try {
            $requestPath = Join-Path $runDirectory 'watchdog.request'
            $gameStatePath = Join-Path $runDirectory 'watchdog-state.txt'
            Set-Content -LiteralPath $requestPath -Value 'capture' -Encoding ascii
            $captureDeadline = (Get-Date).AddSeconds(5)
            while ((Get-Date) -lt $captureDeadline -and -not (Test-Path -LiteralPath $gameStatePath)) { Start-Sleep -Milliseconds 100 }
            if (Test-Path -LiteralPath $gameStatePath) {
                $gameState = Get-Content -Raw -LiteralPath $gameStatePath
                Add-Content -LiteralPath $diagPath -Value "game_state=$gameState"
                $sidecar = Get-ChildItem (Join-Path $runDirectory 'shots\dev') -Recurse -Filter 'devshot-*.json' -File -ErrorAction SilentlyContinue |
                    Sort-Object LastWriteTime -Descending | Select-Object -First 1
                if ($sidecar) {
                    $metadata = Get-Content -Raw -LiteralPath $sidecar.FullName | ConvertFrom-Json
                    $sourceImage = Join-Path $sidecar.DirectoryName $metadata.image
                    if (Test-Path -LiteralPath $sourceImage) {
                        Copy-Item -LiteralPath $sourceImage -Destination $shotPath -Force
                        Copy-Item -LiteralPath $sidecar.FullName -Destination (Join-Path $runDirectory 'watchdog.json') -Force
                        Add-Content -LiteralPath $diagPath -Value "screenshot=$shotPath"
                    } else { Add-Content -LiteralPath $diagPath -Value 'screenshot=engine capture image missing' }
                } else { Add-Content -LiteralPath $diagPath -Value 'screenshot=engine capture metadata missing' }
            } else {
                Add-Content -LiteralPath $diagPath -Value 'game_state=engine watchdog capture timed out'
            }
            if (-not (Test-Path -LiteralPath $shotPath)) {
                $lastImageMeta = Get-ChildItem (Join-Path $runDirectory 'shots\dev') -Recurse -Filter 'devshot-*.json' -File -ErrorAction SilentlyContinue |
                    Sort-Object LastWriteTime -Descending | Select-Object -First 1
                if ($lastImageMeta) {
                    $metadata = Get-Content -Raw -LiteralPath $lastImageMeta.FullName | ConvertFrom-Json
                    $sourceImage = Join-Path $lastImageMeta.DirectoryName $metadata.image
                    if (Test-Path -LiteralPath $sourceImage) {
                        Copy-Item -LiteralPath $sourceImage -Destination $shotPath -Force
                        Copy-Item -LiteralPath $lastImageMeta.FullName -Destination (Join-Path $runDirectory 'watchdog.json') -Force
                        Add-Content -LiteralPath $diagPath -Value "screenshot=last game snapshot before timeout ($shotPath)"
                    }
                }
            }
        } catch { Add-Content -LiteralPath $diagPath -Value "screenshot=capture failed: $($_.Exception.Message)" }
        # The game may still hold its redirected stdout open here (a fast
        # exit races this write); the diagnostic line is best-effort.
        try { Add-Content -LiteralPath $watchdogStdout -Value $diag } catch { }
        if ($OutputDirectory) {
            Copy-Item -LiteralPath $diagPath -Destination $OutputDirectory -Force
            if (Test-Path -LiteralPath $shotPath) { Copy-Item -LiteralPath $shotPath -Destination $OutputDirectory -Force }
            foreach ($watchdogArtifact in @((Join-Path $runDirectory 'watchdog.json'), $gameStatePath)) {
                if (Test-Path -LiteralPath $watchdogArtifact) { Copy-Item -LiteralPath $watchdogArtifact -Destination $OutputDirectory -Force }
            }
        }
        Stop-Process -Id $process.Id -Force
        $probeHarnessStoppedProcess = $true
    }
    Start-Sleep -Milliseconds 500
} finally {
    $env:F2CE_DEV_BACKGROUND = $previousBackground
    $env:SDL_AUDIODRIVER = $previousAudio
    $env:F2CE_SCENE_CACHE = $previousSceneCache
    $env:F2CE_DEV_OUT = $previousOutput
    if ($null -ne $process) {
        if (-not $process.HasExited -and $HoldSeconds -gt 0) {
            # A delayed in-game capture (the owner shot) needs the game alive
            # past the last snapshot.
            Start-Sleep -Seconds $HoldSeconds
        }
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id -Force
            $probeHarnessStoppedProcess = $true
        }
        if (-not $process.WaitForExit(10000)) { throw 'The background test process did not exit.' }
    }
    Release-TestSlot
}
if ($DevInput -or $SceneInput) {
    $inputLog = Get-Content -Raw -LiteralPath (Join-Path $OutputDirectory 'process.stdout.log')
    $inputErrors = Get-Content -Raw -LiteralPath (Join-Path $OutputDirectory 'process.stderr.log')
    if ($inputLog -notmatch '(?m)^DEV_INPUT PASS script complete' -or $inputErrors -match 'DEV_INPUT FAIL' -or $snapshots.Count -ne $DevInputShots) {
        throw "Scripted input did not complete with exactly $DevInputShots snapshots."
    }
    if ($DevInput -and $Preferences -and $ConfiguredDisplay) {
        # The options-panel resolution script must have saved both a new
        # resolution and a new scale into the isolated config.
        $afterWidth = Get-TestConfigValue $configPath 'screen' 'resolution_x'
        $afterHeight = Get-TestConfigValue $configPath 'screen' 'resolution_y'
        $afterScale = Get-TestConfigValue $configPath 'screen' 'scale_factor'
        if (($afterWidth -eq $initialDisplayWidth -and $afterHeight -eq $initialDisplayHeight) -or $afterScale -eq $initialDisplayScale) {
            throw 'Scripted input did not save both a different resolution and scale.'
        }
        [ordered]@{
            coordinate_space = 'logical'; snapshots = $snapshots.Count
            initial = @{ resolution_x = $initialDisplayWidth; resolution_y = $initialDisplayHeight; scale_factor = $initialDisplayScale }
            saved = @{ resolution_x = $afterWidth; resolution_y = $afterHeight; scale_factor = $afterScale }
            config_sha256 = (Get-FileHash -LiteralPath $configPath -Algorithm SHA256).Hash
        } | ConvertTo-Json -Depth 4 | Set-Content -Encoding utf8 -LiteralPath (Join-Path $OutputDirectory 'input-result.json')
    }
}
if ($watchdogExpired) { throw "Live check watchdog: $watchdogReason. Diagnostics: $(Join-Path $runDirectory 'watchdog.txt')" }
if ($SavePreviewFixture) {
    $savePreviewLog = Get-Content -Raw -LiteralPath (Join-Path $OutputDirectory 'process.stdout.log')
    if ($savePreviewLog -notmatch '(?m)^SAVE_PREVIEW_FIXTURE PASS ' -or $savePreviewLog -match '(?m)^SAVE_PREVIEW_FIXTURE FAIL ') {
        throw "Save preview fixture failed or timed out within $TimeoutSeconds s.`n$savePreviewLog"
    }
    if ($snapshots.Count -ne 2) { throw "Save preview fixture must produce exactly two save and load screen snapshots, got $($snapshots.Count)." }
}
if ($ExistingSavePreviewFixture) {
    $existingPreviewLog = Get-Content -Raw -LiteralPath (Join-Path $OutputDirectory 'process.stdout.log')
    if ($existingPreviewLog -notmatch '(?m)^EXISTING_SAVE_PREVIEW_FIXTURE PASS ' -or $existingPreviewLog -match '(?m)^EXISTING_SAVE_PREVIEW_FIXTURE FAIL ') {
        throw "Existing save preview fixture failed or timed out within $TimeoutSeconds s.`n$existingPreviewLog"
    }
    if ($snapshots.Count -ne 1) { throw "Existing save preview fixture must produce exactly one load-screen snapshot, got $($snapshots.Count)." }
}
if ($MuzzleWeapon -gt 0) {
    $muzzleText086=Get-Content -Raw -LiteralPath (Join-Path $OutputDirectory 'process.stdout.log')
    if ($muzzleText086 -notmatch 'MUZZLE_FIXTURE PASS' -or $muzzleText086 -match 'MUZZLE_FIXTURE FAIL') {throw 'Muzzle batch did not finish successfully.'}
}
if ($ClockLadder) {
    $clockText094=Get-Content -Raw -LiteralPath (Join-Path $OutputDirectory 'process.stdout.log')
    if ($clockText094 -notmatch '(?m)^CLOCK_LADDER PASS') { throw 'Clock ladder did not finish successfully.' }
}
if ($ScrollStep -or $ScrollEdge -or $FollowHero -or $WalkProbe) {
    $scrollText = Get-Content -Raw -LiteralPath (Join-Path $OutputDirectory 'process.stdout.log')
    if ($ScrollEdge -and $scrollText -notmatch '(?m)^SCROLL_EDGE PASS ') { throw 'Screen-edge scroll did not finish successfully.' }
    if ($FollowHero -and $scrollText -notmatch '(?m)^FOLLOW_HERO PASS ') { throw 'Hero-centering probe did not finish successfully.' }
    if ($WalkProbe -and $scrollText -notmatch '(?m)^WALK_PROBE RESULT ') { throw 'Walk probe did not finish.' }
    if ($ScrollStep -and $scrollText -notmatch '(?m)^SCROLL_STEP PASS ') { throw 'Scroll step did not finish successfully.' }
}
if ($PreferencesProbe) {
    $probeLog = Get-Content -Raw -LiteralPath (Join-Path $OutputDirectory 'process.stdout.log')
    if (-not $probeCompletedBeforeDeadline) { throw "Preferences probe did not finish within $TimeoutSeconds s." }
    if (($probeLog -match 'PREFERENCES_PROBE FAIL') -or
        ($probeLog -notmatch 'PREFERENCES_PROBE PASS') -or
        ($probeLog -notmatch 'PREFERENCES_PROBE finished; requesting isolated helper exit')) {
        throw 'Preferences probe did not report a successful production-path assertion.'
    }
    if (-not $probeHarnessStoppedProcess) { throw 'Preferences probe did not stop through the isolated helper harness.' }
    $configHashAfter = (Get-FileHash -LiteralPath $configPath -Algorithm SHA256).Hash
    $zoomAfter = Get-TestConfigValue $configPath '3d' 'zoom'
    $enabledAfter = Get-TestConfigValue $configPath '3d' 'enabled'
    $difficultyAfter = Get-TestConfigValue $configPath 'preferences' 'combat_difficulty'
    switch ($PreferencesProbe) {
        { $_ -in 'cancel', 'cancel-key', 'escape', 'verify' } {
            if ($configHashAfter -ne $configHashBefore) { throw "$PreferencesProbe changed the isolated config hash." }
        }
        { $_ -in 'save', 'save-key' } {
            $zoomMatch = [regex]::Match($probeLog, 'PREFERENCES_PROBE PASS committed zoom=([0-9.]+) enabled=(true|false) combat_difficulty=(-?\d+)')
            if (-not $zoomMatch.Success) { throw 'Done commit values were not reported.' }
            $expectedEnabled = if ($zoomMatch.Groups[2].Value -eq 'true') { '1' } else { '0' }
            if ([math]::Abs(([double]$zoomAfter) - ([double]$zoomMatch.Groups[1].Value)) -gt 0.00001 -or
                $enabledAfter -ne $expectedEnabled -or $difficultyAfter -ne $zoomMatch.Groups[3].Value -or
                $difficultyAfter -eq $initialDifficulty) {
                throw 'Saved config does not contain the committed 3D and legacy values.'
            }
        }
        'defaults' {
            if ([math]::Abs(([double]$zoomAfter) - 1.0) -gt 0.00001 -or
                $enabledAfter -ne '1' -or $difficultyAfter -ne '1') {
                throw 'Defaults did not write the expected 3D and legacy defaults.'
            }
        }
        'verify' {
            $readback = [regex]::Match($probeLog, 'PREFERENCES_PROBE READBACK zoom=([0-9.]+) enabled=(true|false) combat_difficulty=(-?\d+)')
            if (-not $readback.Success) { throw 'Fresh-process readback values were not reported.' }
            $expectedEnabled = if ($readback.Groups[2].Value -eq 'true') { '1' } else { '0' }
            if ([math]::Abs(([double]$zoomAfter) - ([double]$readback.Groups[1].Value)) -gt 0.00001 -or
                $enabledAfter -ne $expectedEnabled -or $difficultyAfter -ne $readback.Groups[3].Value) {
                throw 'Fresh process did not load the saved values from the isolated config.'
            }
        }
    }
    Write-Output $probeLog
    Write-Output "PREFERENCES_PROBE CONFIG $PreferencesProbe hash_before=$configHashBefore hash_after=$configHashAfter zoom=$zoomAfter enabled=$enabledAfter combat_difficulty=$difficultyAfter"
    $probeResult = [ordered]@{
        mode = $PreferencesProbe
        config_sha256_before = $configHashBefore
        config_sha256_after = $configHashAfter
        initial = [ordered]@{
            zoom = $initialZoom
            enabled = $initialEnabled
            combat_difficulty = $initialDifficulty
        }
        final = [ordered]@{
            zoom = $zoomAfter
            enabled = $enabledAfter
            combat_difficulty = $difficultyAfter
        }
        executable_sha256 = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
        completion = [ordered]@{
            method = 'C++ finish marker observed before deadline; isolated helper stopped by harness'
            marker_observed_before_deadline = $probeCompletedBeforeDeadline
            harness_stopped_process = $probeHarnessStoppedProcess
        }
    }
    $resultPath = Join-Path $OutputDirectory 'preferences-result.json'
    $probeResult | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $resultPath -Encoding utf8
    Write-Output "PREFERENCES_PROBE RESULT $resultPath"
    Complete-RunFolder
    return
}
if ($UiFixture) {
    $fixtureLog = Get-Content -Raw -LiteralPath (Join-Path $OutputDirectory 'process.stdout.log')
    if ($UiFixture -eq 'bark') {
        if ($dialogFixtureFailed -or $fixtureLog -notmatch '(?m)^UI_FIXTURE PASS screen=bark first=1 replacement=1 cleared_runs=0\r?$') {
            throw "Bark fixture failed to replace and expire its outlined text.`n$fixtureLog"
        }
        $visibleBarkLine = [regex]::Match($fixtureLog, '(?m)^UI_FIXTURE STEP screen=bark stage=visible hasOutline=1 visiblePixels=(\d+) ')
        $replacementBarkLine = [regex]::Match($fixtureLog, '(?m)^UI_FIXTURE STEP screen=bark stage=replaced hasOutline=1 visiblePixels=(\d+) ')
        if (-not $visibleBarkLine.Success -or [int]$visibleBarkLine.Groups[1].Value -le 0 -or
            -not $replacementBarkLine.Success -or [int]$replacementBarkLine.Groups[1].Value -le 0) {
            throw 'Bark fixture did not prove visible pixels for both outlined Russian runs.'
        }
        if (-not $probeHarnessStoppedProcess -or $snapshots.Count -lt 2) {
            throw 'Bark fixture must capture the visible bark and its cleared frame in the isolated helper process.'
        }
        $ordered = @($snapshots | Sort-Object LastWriteTime)
        $barkMetadata = Get-Content -Raw -LiteralPath $ordered[0].FullName | ConvertFrom-Json
        $clearMetadata = Get-Content -Raw -LiteralPath $ordered[-1].FullName | ConvertFrom-Json
        $expectedResolution = "${ScreenWidth}x${ScreenHeight}"
        $expectedLogicalResolution = "${logicalWidth}x${logicalHeight}"
        $expectedLegacyScale = [int][Math]::Round($Scale)
        foreach ($metadata in @($barkMetadata, $clearMetadata)) {
            if ($metadata.snapshot_run -ne $runId -or $metadata.resolution -ne $expectedResolution -or
                $metadata.logical_resolution -ne $expectedLogicalResolution -or $metadata.scale -ne $expectedLegacyScale -or
                [Math]::Abs([double]$metadata.scale_factor - $Scale) -gt 0.01) {
                throw 'Bark fixture metadata does not match the requested dimensions and scale.'
            }
        }
        if ($SharpText) {
            if (-not $barkMetadata.sharp_text.setting -or -not $barkMetadata.sharp_text.active -or
                -not $barkMetadata.sharp_text.drawn -or $barkMetadata.sharp_text.runs -le 0) {
                throw 'Sharp-on bark snapshot lacks a drawn font-101 pass.'
            }
        } elseif ($barkMetadata.sharp_text.setting -or $barkMetadata.sharp_text.active -or $barkMetadata.sharp_text.drawn) {
            throw 'Sharp-off bark snapshot unexpectedly reports active SharpText.'
        }
    } else {
    if ($dialogFixtureFailed -or $fixtureLog -notmatch ('(?m)^UI_FIXTURE PASS screen=' + [regex]::Escape($UiFixture) + ' ')) {
        throw "UI fixture '$UiFixture' failed or timed out within $TimeoutSeconds s.`n$fixtureLog"
    }
    if (-not $probeHarnessStoppedProcess) { throw 'UI fixture was not stopped by its isolated helper harness.' }
    if ($snapshots.Count -lt 1) { throw "UI fixture '$UiFixture' passed without a framebuffer snapshot." }
    $uiMetadata = Get-Content -Raw -LiteralPath $snapshots[0].FullName | ConvertFrom-Json
    $expectedResolution = "${ScreenWidth}x${ScreenHeight}"
    $expectedLogicalResolution = "${logicalWidth}x${logicalHeight}"
    $expectedLegacyScale = [int][Math]::Round($Scale)
    if ($uiMetadata.snapshot_run -ne $runId -or $uiMetadata.resolution -ne $expectedResolution -or
        $uiMetadata.logical_resolution -ne $expectedLogicalResolution -or $uiMetadata.scale -ne $expectedLegacyScale -or
        [Math]::Abs([double]$uiMetadata.scale_factor - $Scale) -gt 0.01) {
        throw "UI fixture '$UiFixture' snapshot metadata does not match the requested run, dimensions, and scale."
    }
    $passLine = [regex]::Match($fixtureLog, '(?m)^UI_FIXTURE PASS screen=' + [regex]::Escape($UiFixture) + ' window=(\d+) generation=(\d+) font=101 runs=(\d+) screen_runs=(\d+) visible_runs=(\d+) visible_coverage=(\d+) overlay_runs=(\d+) overlay_pixels=(\d+)')
    if (-not $passLine.Success -or [int]$passLine.Groups[3].Value -le 0 -or
        [int]$passLine.Groups[4].Value -le 0 -or [int]$passLine.Groups[5].Value -le 0 -or
        [int]$passLine.Groups[6].Value -le 0) {
        throw "UI fixture '$UiFixture' lacks a visible target-window font-101 record and coverage proof."
    }
    if ($Font101PreviewScale -and $fixtureLog -notmatch ('(?m)^UI_FIXTURE PASS screen=character .* preview_scale=' + [regex]::Escape($Font101PreviewScale) + '(?: |$)')) {
        throw "Character fixture did not confirm requested font-101 preview scale $Font101PreviewScale."
    }
    if ($Font101PreviewWeight -and $fixtureLog -notmatch ('(?m)^UI_FIXTURE PASS screen=character .* preview_weight=' + [regex]::Escape($Font101PreviewWeight) + '(?: |$)')) {
        throw "Character fixture did not confirm requested font-101 preview weight $Font101PreviewWeight."
    }
    if ($SharpText -and ($UiFixture -in @('inventory', 'pipboy')) -and
        $fixtureLog -notmatch '(?m)^UI_FIXTURE PASS screen=(inventory|pipboy) .*compact_size_role_runs=\d+ default_size_role_runs=0 run_scale=1\.22\.\.1\.22(?: |$)') {
        throw "Sharp-on $UiFixture fixture did not prove 1.22 CompactUi role on every font-101 run."
    }
    if ($SharpText -and ([int]$passLine.Groups[7].Value -le 0 -or [int]$passLine.Groups[8].Value -le 0)) {
        throw "Sharp-on UI fixture '$UiFixture' lacks visible overlay pixels from its target window."
    }
    if ($UiFixture -eq 'barter' -and $fixtureLog -notmatch '(?m)^UI_FIXTURE PASS .*white_zero_totals=2 left=\$0@\d+ right=\$0@\d+') {
        throw 'Barter fixture did not prove two white $0 totals at distinct screen positions.'
    }
    if ($UiFixture -eq 'log' -and $fixtureLog -notmatch '(?m)^UI_FIXTURE PASS .*real_date_log=.+') {
        throw 'Message log fixture did not report the real date message from editor.msg.'
    }
    if ($SharpText) {
        if (-not $uiMetadata.sharp_text.setting -or -not $uiMetadata.sharp_text.active -or
            -not $uiMetadata.sharp_text.drawn -or $uiMetadata.sharp_text.runs -le 0) {
            throw "Sharp-on UI fixture '$UiFixture' lacks an active drawn font-101 pass in its snapshot metadata."
        }
    } elseif ($uiMetadata.sharp_text.setting -or $uiMetadata.sharp_text.active -or $uiMetadata.sharp_text.drawn) {
        throw "Sharp-off UI fixture '$UiFixture' unexpectedly reports active sharp text."
    }
    }
}
if ($DialogFixture -and -not $DialogPageInspect -and -not $DialogCursorCheck) {
    $fixtureLog = Get-Content -Raw -LiteralPath (Join-Path $OutputDirectory 'process.stdout.log')
    if ($dialogFixtureFailed) {
        throw "Dialog fixture reported failure.`n$fixtureLog"
    }
    if ($fixtureLog -notmatch '(?m)^DIALOG_FIXTURE PASS ') {
        throw "Dialog fixture timed out within $TimeoutSeconds s.`n$fixtureLog"
    }
    if (-not $probeHarnessStoppedProcess) { throw 'Dialog fixture was not stopped by its isolated helper harness.' }
}
if ($DialogPageInspect) {
    $inspectLog = Get-Content -Raw -LiteralPath (Join-Path $OutputDirectory 'process.stdout.log')
    if ($inspectLog -notmatch '(?m)^DIALOG_PAGE_INSPECT PASS .*reply_offset=\d+') {
        throw "Dialog page inspection did not report a completed real-key selection within $TimeoutSeconds s.`n$inspectLog"
    }
    if (-not $probeHarnessStoppedProcess) { throw 'Dialog page inspection was not stopped by its isolated helper harness.' }
    Write-Output $inspectLog
    Complete-RunFolder
    return
}
if ($DialogCursorCheck) {
    $cursorLog = Get-Content -Raw -LiteralPath (Join-Path $OutputDirectory 'process.stdout.log')
    $cursorPass = [regex]::Match($cursorLog, '(?m)^DIALOG_CURSOR_CHECK PASS .*opaque_physical=(\d+),(\d+).*transparent_physical=(\d+),(\d+).*cursor_index=\d+ expected_rgb=(\d+),(\d+),(\d+).*opaque_alpha=(\d+)->0 transparent_alpha=(\d+)->(\d+) same_run_ink_pixels=(\d+)->(\d+)')
    if ($dialogFixtureFailed -or -not $cursorPass.Success) {
        throw "Dialog cursor check failed or timed out within $TimeoutSeconds s.`n$cursorLog"
    }
    if ([int]$cursorPass.Groups[8].Value -le 0 -or [int]$cursorPass.Groups[9].Value -le 0 -or
        [int]$cursorPass.Groups[9].Value -ne [int]$cursorPass.Groups[10].Value -or
        [int]$cursorPass.Groups[11].Value -le [int]$cursorPass.Groups[12].Value -or
        [int]$cursorPass.Groups[12].Value -le 0) {
        throw 'Dialog cursor check did not preserve sharp text at the transparent cursor cell.'
    }
    if (-not $probeHarnessStoppedProcess) { throw 'Dialog cursor check was not stopped by its isolated helper harness.' }
    if ($snapshots.Count -lt 1) { throw 'Dialog cursor check passed without a framebuffer snapshot.' }
    $cursorMetadata = Get-Content -Raw -LiteralPath $snapshots[0].FullName | ConvertFrom-Json
    if (-not $cursorMetadata.sharp_text.active -or -not $cursorMetadata.sharp_text.drawn -or
        $cursorMetadata.sharp_text.runs -le 0 -or $cursorMetadata.sharp_text.glyphs -le 0) {
        throw 'Dialog cursor snapshot does not prove an active drawn sharp-text pass.'
    }
    $cursorImagePath = Get-SnapshotImage $snapshots[0] $cursorMetadata
    & py -3 (Join-Path $PSScriptRoot 'check_png_pixel.py') $cursorImagePath $cursorPass.Groups[1].Value $cursorPass.Groups[2].Value `
        $cursorPass.Groups[5].Value $cursorPass.Groups[6].Value $cursorPass.Groups[7].Value
    if ($LASTEXITCODE -ne 0) { throw 'Dialog cursor pixel in the decoded PNG does not match the engine cursor palette color.' }
    Write-Output $cursorLog
}
if ($DialogLayoutCheck) {
    $layoutLog = Get-Content -Raw -LiteralPath (Join-Path $OutputDirectory 'process.stdout.log')
    if ($dialogFixtureFailed -or $layoutLog -notmatch '(?m)^DIALOG_LAYOUT_CHECK PASS geometry=non-overlapping buttons=7 text_rows=8') {
        throw "Dialog layout geometry check failed or timed out within $TimeoutSeconds s.`n$layoutLog"
    }
}
if ($snapshots.Count -lt $shots) { throw "No snapshot within $TimeoutSeconds s." }
if ($OutputDirectory) { New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null }
foreach ($snapshot in $snapshots) {
    $shown = $snapshot.FullName
    if ($OutputDirectory) {
        $metadata = Get-Content -Raw -LiteralPath $snapshot.FullName | ConvertFrom-Json
        Copy-Item -LiteralPath $snapshot.FullName -Destination $OutputDirectory
        Copy-Item -LiteralPath (Get-SnapshotImage $snapshot $metadata) -Destination $OutputDirectory
        $shown = Join-Path $OutputDirectory $snapshot.Name
    }
    Write-Output ("SNAPSHOT: " + $shown)
    Get-Content $snapshot.FullName
}
$probeLog = Join-Path $runDirectory 'logs\devprobe.log'
if ($OutputDirectory) {
    # -ShadowDebug 6 writes the depth map dump.
    foreach ($kept in @($probeLog, (Join-Path $runDirectory 'shadowmap-dump.pgm'))) {
        if (Test-Path -LiteralPath $kept) { Copy-Item -LiteralPath $kept -Destination $OutputDirectory }
    }
}
if ($Probe) {
    Write-Output "PROBE:"
    Get-Content $probeLog
}
Complete-RunFolder
