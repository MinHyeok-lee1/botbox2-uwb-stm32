[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$failCount = 0
$warnCount = 0

function Write-Result {
    param([ValidateSet('PASS', 'FAIL', 'WARN')][string]$Level, [string]$Message)
    if ($Level -eq 'FAIL') { $script:failCount++ }
    if ($Level -eq 'WARN') { $script:warnCount++ }
    Write-Host "[$Level] $Message"
}

function Get-RelativePath {
    param([string]$Path)
    $baseUri = [Uri]((Resolve-Path -LiteralPath $repo).Path.TrimEnd('\') + '\')
    $pathUri = [Uri](Resolve-Path -LiteralPath $Path).Path
    return [Uri]::UnescapeDataString($baseUri.MakeRelativeUri($pathUri).ToString()).Replace('/', '\')
}

Write-Host '=== 03 Customer lightweight boundary scan ==='
$items = @(Get-ChildItem -LiteralPath $repo -Recurse -Force -ErrorAction SilentlyContinue | Where-Object { $_.FullName -notmatch '[\\/]\.git(?:[\\/]|$)' })
$antigravity = @($items | Where-Object { $_.FullName -match '[\\/]docs[\\/]antigravity(?:[\\/]|$)' })
if ($antigravity.Count -gt 0) {
    Write-Result FAIL "Customer contains docs/antigravity content ($($antigravity.Count) path(s))."
    $antigravity | ForEach-Object { Write-Host "       $(Get-RelativePath $_.FullName)" }
} else {
    Write-Result PASS 'Customer contains no docs/antigravity path.'
}

$internalNames = @('task.md', 'PROJECT_MEMORY.md', 'SESSION_START.md')
$internalDocs = @($items | Where-Object { -not $_.PSIsContainer -and $_.Name -cin $internalNames })
if ($internalDocs.Count -gt 0) {
    Write-Result FAIL "Customer contains prohibited internal operating document(s): $($internalDocs.Count)."
    $internalDocs | ForEach-Object { Write-Host "       $(Get-RelativePath $_.FullName)" }
} else {
    Write-Result PASS 'Customer contains no prohibited internal operating documents.'
}

Write-Host "=== Summary: FAIL=$failCount, WARN=$warnCount ==="
if ($failCount -gt 0) { exit 1 }
exit 0
