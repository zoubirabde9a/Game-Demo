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
#    $MaxLines (600 until 2026-10-06). The files below were already
#    longer when the limit came in; each may shrink but not grow past its
#    listed size. Split a file rather than raising a number here.
# 4. Every source file is included by another one, except the programs'
#    entry points (code/app.cpp, platform/*_app.cpp, server/*_main.cpp and
#    the files directly in tests/ and tools/). A file nothing includes is
#    dead code that still costs a reader's time.
# 5. Every .cpp and .inc outside third_party/ starts with a comment
#    saying what it is for, so a reader can stop at the top of the file.
# 6. No #if 0 blocks. Code switched off for good is deleted (git keeps
#    it); a debug view someone wants back goes behind a named #define.
param([string]$Root = (Split-Path -Parent $PSScriptRoot))
$Root = [System.IO.Path]::GetFullPath($Root)

$MaxLines = 500
$Ceilings = @{
    'code/engine/random.h'                  = 604  # mostly a table of random numbers
    'code/engine/math.h'                    = 648  # about 50 small vector helpers
    'code/art/terrain_art.cpp'              = 945  # claimed by the terrain agent
    'code/art/monster_render.cpp'           = 502  # claimed by the monster agent
    'code/sim/terrain/maps.cpp'             = 514  # claimed by the terrain agent
    'code/client/draw_tilemap.cpp'          = 521  # claimed by the terrain agent
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

# 4. Files nothing includes.
$Sources = @(Get-ChildItem -Path $Code -Recurse -Include '*.cpp', '*.h', '*.inc' |
    Where-Object { $_.FullName -notmatch '[\\/]third_party[\\/]' })
$Included = @{}
foreach ($File in $Sources) {
    foreach ($Line in Get-Content $File.FullName) {
        if ($Line -match '^\s*#\s*include\s+"([^"]*)"') {
            $Target = [System.IO.Path]::GetFullPath((Join-Path $File.DirectoryName $Matches[1]))
            $Included[$Target.ToLowerInvariant()] = $true
        }
    }
}
foreach ($File in $Sources) {
    $Name = RelativePath $File.FullName
    $EntryPoint = $Name -eq 'code/app.cpp' -or
        $Name -match '^code/platform/[^/]*_app\.cpp$' -or
        $Name -match '^code/server/[^/]*_main\.cpp$' -or
        $Name -match '^code/(tests|tools)/[^/]*\.cpp$'
    if (-not $EntryPoint -and -not $Included.ContainsKey($File.FullName.ToLowerInvariant())) {
        $Failures.Add("${Name}: nothing includes it; add it to its module's list or delete it")
    }
}

# 5. A comment at the top of every file.
# A header may put its include guard (#if !defined(X) / #define X) first.
# The old empty template banner ($File: $, $Date: $) does not count.
foreach ($File in $Sources) {
    $Lines = @(Get-Content $File.FullName -TotalCount 12 |
        Where-Object { $_.Trim() -ne '' -and $_ -notmatch '^\s*#\s*(if\s*!\s*defined|ifndef|define)\b' })
    $First = if ($Lines.Count -gt 0) { $Lines[0] } else { '' }
    $Name = RelativePath $File.FullName
    if ($First -notmatch '^\s*(/\*|//)') {
        $Failures.Add("${Name}: start the file with a comment saying what it is for")
    }
    # NOTE(zoubir): sim/world.* are in the terrain agent's claim; until it
    # writes their summaries (.agents/issues/world-banners.md) they pass
    elseif (($Lines -join "`n") -match '\$File: \$' -and
            $Name -notin @('code/sim/world.cpp', 'code/sim/world.h')) {
        $Failures.Add("${Name}: replace the empty `$File: `$ banner with a comment saying what the file is for")
    }
}

# 6. Disabled blocks.
foreach ($File in $Sources) {
    $LineNumber = 0
    foreach ($Line in Get-Content $File.FullName) {
        $LineNumber++
        if ($Line -match '^\s*#\s*if\s+0(\s|$)') {
            $Failures.Add("$(RelativePath $File.FullName):${LineNumber}: #if 0 block; delete code that is switched off for good")
        }
    }
}

if ($Failures.Count -gt 0) {
    Write-Output "layout_check: $($Failures.Count) problem(s)"
    $Failures | ForEach-Object { Write-Output "  $_" }
    exit 1
}
Write-Output 'layout_check: ok'
exit 0
