# Lists the open claims in .agents/claims (who, how old, what) and warns
# when this branch changes a file someone else has claimed; then lists the
# open bug reports in .agents/issues and flags those in files you claimed
# or this branch changes. See AGENTS.md section 2. Run from your worktree:
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File misc\claims.ps1
#
# A claim is yours when its "who:" contains your branch name after
# "agent/" (branch agent/netcode, who: claude-netcode). Claims older than
# a day are marked stale; AGENTS.md lets you delete those. Only warns: the
# exit code is 0 unless -Strict is given and a claimed file was touched.
param(
    [string]$Base = "main",
    # who you are in claims' who: lines; default: the branch name after agent/
    [string]$Me = "",
    [switch]$Strict
)

$Root = Split-Path -Parent $PSScriptRoot
$Branch = (git -C $Root rev-parse --abbrev-ref HEAD).Trim()
if (-not $Me) { $Me = if ($Branch -like "agent/*") { $Branch.Substring(6) } else { $Branch } }
$Changed = @(git -C $Root diff --name-only "$Base...HEAD" | Where-Object { $_ })

# The claims as main has them: a claim lives on main, not in the branch.
$ClaimDir = Join-Path $Root ".agents/claims"
$Claims = @()
foreach ($File in Get-ChildItem $ClaimDir -Filter *.md | Where-Object { $_.Name -ne "README.md" }) {
    $Claim = @{ Name = $File.BaseName; Who = ""; Task = ""; Since = ""; Patterns = @() }
    foreach ($Line in Get-Content $File.FullName) {
        if ($Line -match '^(\w+):\s*(.*)$') {
            $Key = $Matches[1]; $Value = $Matches[2]
            switch ($Key) {
                "who"   { $Claim.Who = $Value }
                "task"  { $Claim.Task = $Value }
                "since" { $Claim.Since = $Value }
                "files" {
                    # "a/b.cpp, c/*, later d.cpp (notes)": keep the path-like words
                    $Clean = $Value -replace '\([^)]*\)', ' '
                    $Claim.Patterns = @($Clean -split '[,;\s]+' |
                        Where-Object { $_ -match '[/\\.]' } |
                        ForEach-Object { $_.Trim().Replace('\', '/') })
                }
            }
        }
    }
    $Claims += $Claim
}

$Today = Get-Date
$Warnings = @()
$MyPatterns = @()
Write-Host "claims: $($Claims.Count) open"
foreach ($Claim in $Claims) {
    $Age = ""
    $Stale = $false
    $SinceDate = [datetime]::MinValue
    if ([datetime]::TryParse($Claim.Since, [ref]$SinceDate)) {
        $Days = [int]($Today.Date - $SinceDate.Date).TotalDays
        $Age = if ($Days -eq 0) { "today" } else { "$Days d" }
        $Stale = $Days -gt 1
    }
    $Mine = $Me -and $Claim.Who -like "*$Me*"
    $Task = $Claim.Task
    if ($Task.Length -gt 70) { $Task = $Task.Substring(0, 67) + "..." }
    $Flags = @()
    if ($Mine) { $Flags += "yours" }
    if ($Stale) { $Flags += "STALE" }
    $FlagText = if ($Flags.Count) { " [" + ($Flags -join ", ") + "]" } else { "" }
    Write-Host ("  {0,-18} {1,-24} {2,-6} {3}{4}" -f $Claim.Name, $Claim.Who, $Age, $Task, $FlagText)

    if ($Mine) { $MyPatterns += $Claim.Patterns; continue }
    foreach ($Path in $Changed) {
        foreach ($Pattern in $Claim.Patterns) {
            if ($Path -like $Pattern) {
                $Warnings += "  $Path is claimed by $($Claim.Who) ($($Claim.Name))"
                break
            }
        }
    }
}

# Open bug reports, one file each (AGENTS.md section 2).
$IssueDir = Join-Path $Root ".agents/issues"
$Issues = @(Get-ChildItem $IssueDir -Filter *.md -ErrorAction SilentlyContinue | Where-Object { $_.Name -ne "README.md" })
if ($Issues.Count) {
    Write-Host "issues: $($Issues.Count) open"
    foreach ($File in $Issues) {
        $What = ""; $Files = @(); $Since = ""
        foreach ($Line in Get-Content $File.FullName) {
            if ($Line -match '^what:\s*(.*)$') { $What = $Matches[1] }
            if ($Line -match '^since:\s*(.*)$') { $Since = $Matches[1] }
            if ($Line -match '^files:\s*(.*)$') {
                $Files = @(($Matches[1] -replace '\([^)]*\)', ' ') -split '[,;\s]+' |
                    Where-Object { $_ -match '[/\\.]' } | ForEach-Object { $_.Trim().Replace('\', '/') })
            }
        }
        $ForYou = @()
        foreach ($IssueFile in $Files) {
            foreach ($Pattern in $MyPatterns) {
                if ($IssueFile -like $Pattern -or $Pattern -like $IssueFile) { $ForYou += "in your claim"; break }
            }
            foreach ($Path in $Changed) {
                if ($Path -like $IssueFile) { $ForYou += "your branch touches it"; break }
            }
        }
        $Flag = if ($ForYou.Count) { " [" + (($ForYou | Sort-Object -Unique) -join ", ") + "]" } else { "" }
        if ($What.Length -gt 80) { $What = $What.Substring(0, 77) + "..." }
        Write-Host ("  {0,-24} {1,-10} {2}{3}" -f $File.BaseName, $Since, $What, $Flag)
    }
}

if ($Warnings.Count) {
    Write-Host "claims: this branch changes files other agents claimed:"
    $Warnings | Sort-Object -Unique | ForEach-Object { Write-Host $_ }
    Write-Host "claims: ask the owner (AGENTS.md section 6) or split the part you need first."
    if ($Strict) { exit 1 }
}
exit 0
