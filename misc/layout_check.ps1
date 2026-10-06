# Checks the rules that keep the code readable from the top down
# (docs/architecture-plan.md). test.bat runs it first; it exits 1 and
# names the file when a rule breaks.
#
# 1. Every *_module.cpp starts with a /* comment saying what the module
#    does, its entry points and what it may depend on.
# 2. The code the dedicated server compiles (sim/, net/, server/,
#    engine/engine_core.cpp) includes nothing from client/, ui/, art/ or
#    platform/. That code would break the server build on Linux.
# 3. No source file outside tests/ and third_party/ is longer than
#    $MaxLines. The files below were already longer when the check came in;
#    each may shrink but not grow past its listed size. Split a file
#    rather than raising a number here.
param([string]$Root = (Split-Path -Parent $PSScriptRoot))
$Root = [System.IO.Path]::GetFullPath($Root)

$MaxLines = 600
$Ceilings = @{
    'code/engine/random.h'      = 604  # mostly a table of random numbers
    'code/engine/math.h'        = 648  # about 50 small vector helpers
    'code/server/replay.cpp'    = 604
    'code/client/fx_bursts.cpp' = 708
    'code/art/terrain_art.cpp'  = 945  # claimed by the terrain agent
}

$Code = Join-Path $Root 'code'
$Failures = New-Object System.Collections.Generic.List[string]

function RelativePath($Path) {
    return $Path.Substring($Root.Length + 1).Replace('\', '/')
}

# 1. Module summaries.
Get-ChildItem -Path $Code -Recurse -Filter '*_module.cpp' | ForEach-Object {
    $First = Get-Content $_.FullName -TotalCount 1
    if ($First -notmatch '^\s*/\*') {
        $Failures.Add("$(RelativePath $_.FullName): a module file must start with a /* summary */ of what it does, its entry points and what it depends on")
    }
}

# 2. Server-side code stays free of client code.
$ServerSide = @('sim', 'net', 'server') | ForEach-Object { Join-Path $Code $_ }
$ServerFiles = @(Get-ChildItem -Path $ServerSide -Recurse -Include '*.cpp', '*.h', '*.inc')
$ServerFiles += Get-Item (Join-Path $Code 'engine/engine_core.cpp')
foreach ($File in $ServerFiles) {
    $LineNumber = 0
    foreach ($Line in Get-Content $File.FullName) {
        $LineNumber++
        if ($Line -notmatch '^\s*#\s*include\s+"([^"]*)"') { continue }
        $Included = $Matches[1]
        $Target = [System.IO.Path]::GetFullPath((Join-Path $File.DirectoryName $Included))
        if ($Target.StartsWith($Code + [System.IO.Path]::DirectorySeparatorChar)) {
            $Top = $Target.Substring($Code.Length + 1).Split('\/')[0]
            if ($Top -in @('client', 'ui', 'art', 'platform')) {
                $Failures.Add("$(RelativePath $File.FullName):${LineNumber}: includes $Included; the server compiles this file, so it may not use client, ui, art or platform code")
            }
        }
    }
}

# 3. File length.
Get-ChildItem -Path $Code -Recurse -Include '*.cpp', '*.h', '*.inc' |
    Where-Object { $_.FullName -notmatch '[\\/](tests|third_party)[\\/]' } |
    ForEach-Object {
        $Name = RelativePath $_.FullName
        $Lines = @(Get-Content $_.FullName).Count
        $Limit = if ($Ceilings.ContainsKey($Name)) { $Ceilings[$Name] } else { $MaxLines }
        if ($Lines -gt $Limit) {
            $Failures.Add("${Name}: $Lines lines, the limit is $Limit; split it by concern into a folder next to it (see docs/architecture-plan.md)")
        }
    }

if ($Failures.Count -gt 0) {
    Write-Output "layout_check: $($Failures.Count) problem(s)"
    $Failures | ForEach-Object { Write-Output "  $_" }
    exit 1
}
Write-Output 'layout_check: ok'
exit 0
