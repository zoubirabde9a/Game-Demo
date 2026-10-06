# Prints a map of code/: every folder, and in it every source file with
# its length and the first sentence of the comment at its top (the
# layout check makes sure there is one). Read this before opening files:
# it is usually enough to know which file a change belongs in.
#
#   powershell -File misc\code_map.ps1            the whole tree
#   powershell -File misc\code_map.ps1 client     only folders under code/client
#
# tests/ and third_party/ are left out; their files are lists of cases and
# outside code.
param([string]$Under = '', [string]$Root = (Split-Path -Parent $PSScriptRoot))
$Root = [System.IO.Path]::GetFullPath($Root)
$Code = Join-Path $Root 'code'
$Start = if ($Under) { Join-Path $Code $Under } else { $Code }

# NOTE(zoubir): the first sentence of the file's top comment, on one line
function FirstSentence($Path) {
    $Text = ''
    foreach ($Line in Get-Content $Path -TotalCount 12) {
        $Trimmed = $Line.Trim()
        if ($Trimmed -eq '' -and $Text) { break }
        $Trimmed = $Trimmed -replace '^(/\*+|//+|\*+)\s*', '' -replace '\s*\*/$', ''
        if ($Trimmed -match '^#') { break }
        $Text = ($Text + ' ' + $Trimmed).Trim()
        if ($Text -match '^(.*?\.)(\s|$)') { return $Matches[1] }
    }
    return $Text
}

$Files = Get-ChildItem -Path $Start -Recurse -Include '*.cpp', '*.h', '*.inc' |
    Where-Object { $_.FullName -notmatch '[\\/](tests|third_party)[\\/]' } |
    Sort-Object { $_.DirectoryName }, Name
$Folder = ''
foreach ($File in $Files) {
    $Relative = $File.DirectoryName.Substring($Root.Length + 1).Replace('\', '/')
    if ($Relative -ne $Folder) {
        $Folder = $Relative
        Write-Output ''
        Write-Output "$Folder/"
    }
    $Lines = @(Get-Content $File.FullName).Count
    $About = FirstSentence $File.FullName
    if ($About.Length -gt 110) { $About = $About.Substring(0, 107) + '...' }
    Write-Output ('  {0,-28} {1,4}  {2}' -f $File.Name, $Lines, $About)
}
