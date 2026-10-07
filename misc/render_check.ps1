# Checks that the game still draws what it drew: plays a few fixed scenes
# (a map, scripted keys, a frame), screenshots each, and compares it with
# the reference in misc\render_refs.txt.
#   powershell -File misc\render_check.ps1           compare
#   powershell -File misc\render_check.ps1 -Update   write the references anew
# On a difference it names the scene and how far it moved, and keeps the new
# picture in build\render_check\ to look at. Run -Update and commit the new
# references along with a change that is meant to look different.
#
# A reference is not the image but its signature: the mean colour of each
# block of a 16 x 9 grid. The GPU now and then draws a pixel or two one
# level off from run to run, which a hash of the file would call a change;
# that moves a block's mean by under a thousandth of a level, while any
# change a player could see moves it by far more than SIGNATURE_TOLERANCE.
#
# The references hold for one machine and the debug build: another GPU,
# driver or display scale draws other pixels, and so does the release
# build. It runs build.bat first (-NoBuild skips that); run -Update once
# on a new machine.
param([switch]$Update, [switch]$NoBuild)

$Root = Split-Path -Parent $PSScriptRoot
# NOTE(zoubir): the references are of the debug build of the code as it is
# now; a stale build, or the release build misc\land.bat leaves behind,
# draws other pixels. So build first, unless told the build is current
if (-not $NoBuild) {
    & "$Root\build.bat" | Out-Null
    if ($LASTEXITCODE -ne 0) { Write-Host "render_check: build.bat failed"; exit 1 }
}
$Refs = "$PSScriptRoot\render_refs.txt"
$Out = "$Root\build\render_check"
New-Item -ItemType Directory -Force $Out | Out-Null

# NOTE(zoubir): name, map, keys (see code/platform/win32/screenshot.cpp),
# frame, and if given the weather and time of day (dry and day otherwise).
# Every scene puts the mouse somewhere (M:x,y): the player aims at it, and
# left free it is wherever the desktop's mouse happens to be. Each scene
# shows the effects of one or two maps at work
$Scenes = @(
    @("arena-cast",   "arena",  "2:F4 3:H M:900,400 30:< 60:A 61:< 90-200:D", 300),
    @("keep-ice",     "keep",   "2:F4 3:H M:700,300 10-28:S 28-110:Q", 120),
    @("wastes-lava",  "ashen",  "2:F4 3:H M:700,300 10-380:Q", 400),
    @("wilds-pond",   "wilds",  "2:F4 3:H M:700,300 10-150:D", 150),
    @("wilds-rain",   "wilds",  "2:F4 3:H M:700,300", 120, "rain"),
    @("crypt-torches","crypt",  "2:F4 3:H M:700,300", 120),
    @("arena-night",  "arena",  "2:F4 3:H M:700,300 10-60:D", 120, "dry", "night"),
    @("wilds-dusk",   "wilds",  "2:F4 3:H M:700,300", 120, "dry", "dusk"),
    @("wastes-prints","ashen",  "2:F4 3:H M:900,400 10-150:D", 160)
)

$GridX = 16
$GridY = 9
# NOTE(zoubir): in colour levels (0..255) of a block's mean
$SIGNATURE_TOLERANCE = 0.05

Add-Type -AssemblyName System.Drawing
# NOTE(zoubir): the mean red, green and blue of each block, row by row
function Get-Signature([string]$Png) {
    $Image = [System.Drawing.Bitmap]::FromFile($Png)
    $Rect = New-Object System.Drawing.Rectangle 0, 0, $Image.Width, $Image.Height
    $Data = $Image.LockBits($Rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly,
                            [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $Bytes = New-Object byte[] ($Data.Stride * $Image.Height)
    [System.Runtime.InteropServices.Marshal]::Copy($Data.Scan0, $Bytes, 0, $Bytes.Length)
    $Image.UnlockBits($Data)
    $Width = $Image.Width; $Height = $Image.Height; $Stride = $Data.Stride
    $Image.Dispose()
    $Sums = New-Object double[] ($GridX * $GridY * 3)
    $Counts = New-Object int[] ($GridX * $GridY)
    for ($Y = 0; $Y -lt $Height; $Y++) {
        $Row = [math]::Floor($Y * $GridY / $Height) * $GridX
        $Base = $Y * $Stride
        for ($X = 0; $X -lt $Width; $X++) {
            $Cell = $Row + [math]::Floor($X * $GridX / $Width)
            $I = $Base + 4 * $X
            $Sums[3 * $Cell] += $Bytes[$I + 2]
            $Sums[3 * $Cell + 1] += $Bytes[$I + 1]
            $Sums[3 * $Cell + 2] += $Bytes[$I]
            $Counts[$Cell]++
        }
    }
    $Result = New-Object double[] $Sums.Length
    for ($I = 0; $I -lt $Sums.Length; $I++) {
        $Result[$I] = $Sums[$I] / $Counts[[math]::Floor($I / 3)]
    }
    return ,$Result
}

$Known = @{}
if (Test-Path $Refs) {
    foreach ($Line in Get-Content $Refs) {
        $Parts = $Line -split '\s+'
        if ($Parts.Count -gt 2) { $Known[$Parts[0]] = [double[]]$Parts[1..($Parts.Count - 1)] }
    }
}

$env:GAME_WINDOW = "1280x720"
$env:GAME_SERVER = "offline"
$Lines = @()
$Differ = @()
foreach ($Scene in $Scenes) {
    $Name = $Scene[0]
    $env:GAME_MAP = $Scene[1]
    $env:GAME_SCREENSHOT_KEYS = $Scene[2]
    $env:GAME_WEATHER = if ($Scene.Count -gt 4) { $Scene[4] } else { "dry" }
    $env:GAME_TIME = if ($Scene.Count -gt 5) { $Scene[5] } else { "day" }
    $Png = "$Out\$Name.png"
    if (Test-Path $Png) { Remove-Item $Png }
    cmd /c "`"$Root\misc\screenshot.bat`" `"$Png`" $($Scene[3])" | Out-Null
    if (-not (Test-Path $Png)) { Write-Host "render_check: $Name drew nothing"; exit 1 }
    $Signature = Get-Signature $Png
    $Lines += "$Name " + (($Signature | ForEach-Object { $_.ToString("F3", [cultureinfo]::InvariantCulture) }) -join ' ')
    if (-not $Update) {
        $Ref = $Known[$Name]
        $Moved = 0.0
        if (-not $Ref -or $Ref.Length -ne $Signature.Length) {
            $Moved = [double]::PositiveInfinity
        } else {
            for ($I = 0; $I -lt $Signature.Length; $I++) {
                $Moved = [math]::Max($Moved, [math]::Abs($Signature[$I] - $Ref[$I]))
            }
        }
        if ($Moved -gt $SIGNATURE_TOLERANCE) { $Differ += "$Name (a block moved by $([math]::Round($Moved, 3)) levels)" }
    }
}

if ($Update) {
    Set-Content $Refs $Lines
    Write-Host "render_check: wrote $($Scenes.Count) references to misc\render_refs.txt"
    exit 0
}
if ($Differ.Count -eq 0) {
    Write-Host "render_check: $($Scenes.Count) scenes draw as before"
    exit 0
}
foreach ($Line in $Differ) { Write-Host "draws differently: $Line, see build\render_check\" }
exit 1
