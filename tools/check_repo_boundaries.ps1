[CmdletBinding()]
param(
    [string[]]$MarkdownFixtureFiles
)

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

function Get-TrackedMarkdownFiles {
    $tracked = @(& git -C $repo ls-files -- '*.md' 2>&1)
    if ($LASTEXITCODE -ne 0) { throw ($tracked -join ' ') }

    $excludedDirectories = '(?i)(^|/)(?:\.git|build|dist|node_modules|vendor|third_party|external|cmake-build-[^/]*)(?:/|$)'
    foreach ($relativePath in $tracked) {
        $normalizedPath = $relativePath.Replace('\', '/')
        if ($normalizedPath -match $excludedDirectories) { continue }

        $fullPath = Join-Path $repo $relativePath
        if (Test-Path -LiteralPath $fullPath -PathType Leaf) { Get-Item -LiteralPath $fullPath }
    }
}

function Test-MarkdownBodyContent {
    $allowlist = @('agents.md')
    $forbiddenPatterns = [ordered]@{
        'Windows user-home absolute path' = '(?i)\b[a-z]:/users/[^\s<>()`]+'
        'Parent workspace or sibling repository path' = '(?i)(?:\.\./)+(?:01_stm32-dw3000-g474re-evb|02_stm32-dw3000-g474re)(?:/|$)|\buwb_stm32/(?:01_stm32-dw3000-g474re-evb|02_stm32-dw3000-g474re)(?:/|$)'
        'Prohibited internal document name or path' = '(?i)\bdocs/antigravity\b|\btask\.md\b|\binternal_reference\.md\b|\bproject_memory\.md\b|\bsession_start\.md\b'
        'Session handoff filename' = '(?i)\b[a-z0-9._-]*_session_handoff_[a-z0-9._-]*\.md\b'
    }

    if ($MarkdownFixtureFiles.Count -gt 0) {
        $files = @($MarkdownFixtureFiles | ForEach-Object { Get-Item -LiteralPath $_ })
        $fixtureMode = $true
    } else {
        $files = @(Get-TrackedMarkdownFiles)
        $fixtureMode = $false
    }

    $violations = @()
    $inspectedCount = 0
    foreach ($file in $files) {
        if ($fixtureMode) {
            $displayPath = $file.FullName
        } else {
            $displayPath = (Get-RelativePath $file.FullName).Replace('\', '/')
            if ($displayPath.ToLowerInvariant() -in $allowlist) { continue }
        }

        $inspectedCount++
        $lineNumber = 0
        foreach ($line in Get-Content -LiteralPath $file.FullName -Encoding UTF8) {
            $lineNumber++
            # Realistic internal paths in code fences are indistinguishable from
            # leaked metadata, so only the explicit file allowlist bypasses checks.
            $normalizedLine = $line.Replace('\', '/').ToLowerInvariant()
            foreach ($entry in $forbiddenPatterns.GetEnumerator()) {
                $match = [regex]::Match($normalizedLine, $entry.Value)
                if ($match.Success) {
                    $violations += [PSCustomObject]@{
                        File = $displayPath
                        Line = $lineNumber
                        Pattern = $entry.Key
                        Match = $match.Value
                    }
                }
            }
        }
    }

    if ($violations.Count -gt 0) {
        Write-Result FAIL "Customer tracked Markdown contains prohibited body content ($($violations.Count) match(es))."
        foreach ($violation in $violations) {
            Write-Host "       $($violation.File):$($violation.Line) [$($violation.Pattern)] matched '$($violation.Match)'"
        }
    } else {
        Write-Result PASS "Customer tracked Markdown body content is sanitized ($inspectedCount file(s) inspected)."
    }
}

function Test-CriticalFixParity {
    $workspaceRoot = Split-Path -Parent $repo
    $parityScript = Join-Path $workspaceRoot 'tools\check_critical_fix_parity.ps1'
    $productRepo = Join-Path $workspaceRoot '02_stm32-dw3000-g474re'

    if (-not (Test-Path -LiteralPath $parityScript -PathType Leaf)) {
        Write-Result WARN "Critical-fix parity checker not found; skipping 02->03 parity scan ($parityScript)."
        return
    }
    if (-not (Test-Path -LiteralPath (Join-Path $productRepo 'Core\Src\main.c') -PathType Leaf)) {
        Write-Result WARN "02 Product repository not found; skipping 02->03 parity scan ($productRepo)."
        return
    }

    $output = @(& powershell -NoProfile -ExecutionPolicy Bypass -File $parityScript -ProductRepo $productRepo -CustomerRepo $repo 2>&1)
    if ($LASTEXITCODE -eq 0) {
        Write-Result PASS 'Critical-fix parity check passes for 02 Product -> 03 Customer.'
    } else {
        Write-Result FAIL 'Critical-fix parity check failed for 02 Product -> 03 Customer.'
        foreach ($line in $output) {
            Write-Host "       $line"
        }
    }
}

Write-Host '=== 03 Customer lightweight boundary scan ==='
Test-MarkdownBodyContent
Test-CriticalFixParity
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
