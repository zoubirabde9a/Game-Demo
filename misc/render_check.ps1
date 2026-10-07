# Checks that the game still draws what it drew: plays a few fixed scenes
# (a map, scripted keys, a frame), screenshots each, and compares the
# image's hash with the one in misc\render_refs.txt. The game draws the
# same frame every time for the same scene, so any change in a hash is a
# change in what the player sees.
#   powershell -File misc\render_check.ps1           compare
#   powershell -File misc\render_check.ps1 -Update   write the hashes anew
# On a difference it names the scene and keeps the new picture in
# build\render_check\, so it can be looked at. Run -Update and commit the
# new hashes along with a change that is meant to look different.
#
# The hashes hold for one machine and one build: another GPU, driver or
# display scale draws other pixels, and so does the release build
# (misc\land.bat leaves build\ holding it): run build.bat first, and
# -Update once on a new machine.
param([switch]$Update)

$Root = Split-Path -Parent $PSScriptRoot
$Refs = "$PSScriptRoot\render_refs.txt"
$Out = "$Root\build\render_check"
New-Item -ItemType Directory -Force $Out | Out-Null

# NOTE(zoubir): name, map, keys (see code/platform/win32/screenshot.cpp),
# frame. Every scene puts the mouse somewhere (M:x,y): the player aims at
# it, and left free it is wherever the desktop's mouse happens to be.
# Each scene shows the effects of one or two maps at work
$Scenes = @(
    @("arena-cast",   "arena",  "2:F4 3:H M:900,400 30:< 60:A 61:< 90-200:D", 300),
    @("keep-ice",     "keep",   "2:F4 3:H M:700,300 10-28:S 28-110:Q", 120),
    @("wastes-lava",  "ashen",  "2:F4 3:H M:700,300 10-380:Q", 400),
    @("wilds-pond",   "wilds",  "2:F4 3:H M:700,300 10-150:D", 150),
    @("wilds-rain",   "wilds",  "2:F4 3:H M:700,300", 120, "rain"),
    @("crypt-torches","crypt",  "2:F4 3:H M:700,300", 120)
)

$Known = @{}
if (Test-Path $Refs) {
    foreach ($Line in Get-Content $Refs) {
        $Parts = $Line -split '\s+'
        if ($Parts.Count -eq 2) { $Known[$Parts[0]] = $Parts[1] }
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
    $Png = "$Out\$Name.png"
    if (Test-Path $Png) { Remove-Item $Png }
    cmd /c "`"$Root\misc\screenshot.bat`" `"$Png`" $($Scene[3])" | Out-Null
    if (-not (Test-Path $Png)) { Write-Host "render_check: $Name drew nothing"; exit 1 }
    $Hash = (Get-FileHash $Png).Hash
    $Lines += "$Name $Hash"
    if (-not $Update -and $Known[$Name] -ne $Hash) { $Differ += $Name }
}

if ($Update) {
    Set-Content $Refs $Lines
    Write-Host "render_check: wrote $($Scenes.Count) hashes to misc\render_refs.txt"
    exit 0
}
if ($Differ.Count -eq 0) {
    Write-Host "render_check: $($Scenes.Count) scenes draw as before"
    exit 0
}
foreach ($Name in $Differ) { Write-Host "draws differently: $Name (build\render_check\$Name.png)" }
exit 1
