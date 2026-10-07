# Checks that the art drawn by code comes out the same every time: runs
# art.bat (which draws every sprite sheet and the terrain atlas into
# build\monster_art), runs the drawing again, and compares each image.
# Art that differs between two runs reads memory it never set, and so
# looks different from one player to the next; the flagstones did, when a
# kind's cells outgrew their layout tables.
#   powershell -File misc\art_check.ps1
# Prints each image that differs; exits 1 if any do.

$Root = Split-Path -Parent $PSScriptRoot
cmd /c "`"$Root\art.bat`"" | Out-Null
if ($LASTEXITCODE -ne 0) { Write-Host "art_check: art.bat failed"; exit 1 }

$Art = "$Root\build\monster_art"
$First = Join-Path ([IO.Path]::GetTempPath()) "game_art_check"
if (Test-Path $First) { Remove-Item $First -Recurse -Force }
Copy-Item $Art $First -Recurse
Push-Location "$Root\build"
.\monster_sheets.exe | Out-Null
Pop-Location

$Differ = @()
$Files = Get-ChildItem $First -File
foreach ($File in $Files) {
    $Again = Join-Path $Art $File.Name
    if (-not (Test-Path $Again) -or
        (Get-FileHash $File.FullName).Hash -ne (Get-FileHash $Again).Hash) {
        $Differ += $File.Name
    }
}
if ($Differ.Count -eq 0) {
    Write-Host "art_check: $($Files.Count) images, the same on two runs"
    exit 0
}
foreach ($Name in $Differ) { Write-Host "differs between runs: $Name" }
exit 1
