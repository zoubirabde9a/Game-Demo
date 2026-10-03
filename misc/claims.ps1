# Lists the open claims in .agents/claims (who, how old, what) and warns
# when this branch changes a file someone else has claimed. See AGENTS.md
# section 2. Run from your worktree:
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File misc\claims.ps1
#
# A claim is yours when its "who:" contains your branch name after
# "agent/" (branch agent/netcode, who: claude-netcode). Claims older than
# a day are marked stale; AGENTS.md lets you delete those. Only warns: the
# exit code is 0 unless -Strict is given and a claimed file was touched.
param(
    [string]$Base = "main",
    [switch]$Strict
)

$Root = Split-Path -Parent $PSScriptRoot
$Branch = (git -C $Root rev-parse --abbrev-ref HEAD).Trim()
$Me = if ($Branch -like "agent/*") { $Branch.Substring(6) } else { $Branch }
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

    if ($Mine) { continue }
    foreach ($Path in $Changed) {
        foreach ($Pattern in $Claim.Patterns) {
            if ($Path -like $Pattern) {
                $Warnings += "  $Path is claimed by $($Claim.Who) ($($Claim.Name))"
                break
            }
        }
    }
}

if ($Warnings.Count) {
    Write-Host "claims: this branch changes files other agents claimed:"
    $Warnings | Sort-Object -Unique | ForEach-Object { Write-Host $_ }
    Write-Host "claims: ask the owner (AGENTS.md section 6) or split the part you need first."
    if ($Strict) { exit 1 }
}
exit 0
