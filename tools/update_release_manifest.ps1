# tools/update_release_manifest.ps1
# Helper tool to preview or apply SHA256 manifest updates for delivery package.
# Use only during 'Release Finalization' step after official binary approval.

param(
    [switch]$Apply
)

$ManifestFile = "delivery/manifest.txt"
$DeliveryDir = "delivery"
$ManifestBaseName = [System.IO.Path]::GetFileName($ManifestFile)

if (-not (Test-Path $ManifestFile)) {
    Write-Host "[MANIFEST-UPDATE][ERROR] Manifest file not found: $ManifestFile" -ForegroundColor Red
    exit 1
}

$Mode = if ($Apply) { "apply" } else { "dry-run" }
Write-Host "[MANIFEST-UPDATE] Running in mode=$Mode..." -ForegroundColor Cyan

$Lines = Get-Content -Path $ManifestFile
$NewContent = @()
$ChangedCount = 0
$MissingCount = 0
$InvalidCount = 0
$UnchangedCount = 0
$ProcessedFiles = @{}

Write-Host "--- Scanning existing manifest entries ---" -ForegroundColor Gray

foreach ($Line in $Lines) {
    $TrimmedLine = $Line.Trim()
    
    # Preserve comments and empty lines
    if ($TrimmedLine -eq "" -or $TrimmedLine.StartsWith("#")) {
        $NewContent += $Line
        continue
    }
    
    # Expected format: <hash> *<relative_path>
    if ($TrimmedLine -match "^([a-fA-F0-9]{64})\s+(\*?)(.+)$") {
        $ExistingHash = $Matches[1].ToUpper()
        $StarPrefix = $Matches[2]
        $RelativePath = $Matches[3].Trim()
        $FullPath = Join-Path $DeliveryDir $RelativePath
        
        $ProcessedFiles[$RelativePath] = $true
        
        if (-not (Test-Path $FullPath)) {
            Write-Host "[MISSING] $RelativePath" -ForegroundColor Red
            $MissingCount++
            $NewContent += $Line
            continue
        }

        # Security check: Ensure path doesn't try to escape delivery/
        $NormalizedPath = [System.IO.Path]::GetFullPath($FullPath)
        $NormalizedDeliveryDir = [System.IO.Path]::GetFullPath($DeliveryDir)
        if (-not $NormalizedPath.StartsWith($NormalizedDeliveryDir)) {
            Write-Host "[FAIL] Path escape detected: $RelativePath" -ForegroundColor Red
            $InvalidCount++
            $NewContent += $Line
            continue
        }

        # Calculate Current Hash
        try {
            $CurrentHash = (Get-FileHash -Path $FullPath -Algorithm SHA256).Hash.ToUpper()
            
            if ($CurrentHash -eq $ExistingHash) {
                Write-Host "[UNCHANGED] $RelativePath" -ForegroundColor Green
                $NewContent += $Line
                $UnchangedCount++
            } else {
                Write-Host "[WOULD_UPDATE] $RelativePath" -ForegroundColor Yellow
                Write-Host "  Old: $ExistingHash" -ForegroundColor Gray
                Write-Host "  New: $CurrentHash" -ForegroundColor Gray
                
                $ChangedCount++
                $NewContent += "$CurrentHash $($StarPrefix)$RelativePath"
            }
        } catch {
            Write-Host "[ERROR] processing $RelativePath" -ForegroundColor Red
            $NewContent += $Line
        }
    } else {
        Write-Host "[INVALID_LINE] $Line" -ForegroundColor Red
        $InvalidCount++
        $NewContent += $Line
    }
}

Write-Host "`n--- Scanning for extra candidates (not in manifest) ---" -ForegroundColor Gray
$ExtraCount = 0
# Get all files in delivery/ except the manifest itself
$AllFiles = Get-ChildItem -Path $DeliveryDir -File -Recurse | Where-Object { $_.Name -ne $ManifestBaseName }

# Calculate the root path of delivery/ to subtract it manually for PS 5.1 compatibility
$DeliveryFullPath = (Get-Item $DeliveryDir).FullName

foreach ($File in $AllFiles) {
    # Manual relative path calculation for PS 5.1 compatibility
    $FileFullPath = $File.FullName
    if ($FileFullPath.StartsWith($DeliveryFullPath)) {
        $RelPath = $FileFullPath.Substring($DeliveryFullPath.Length).TrimStart([System.IO.Path]::DirectorySeparatorChar)
    } else {
        $RelPath = $File.Name
    }
    
    if (-not [string]::IsNullOrEmpty($RelPath) -and -not $ProcessedFiles.ContainsKey($RelPath)) {
        Write-Host "[EXTRA_CANDIDATE] $RelPath (Not tracked in manifest)" -ForegroundColor Magenta
        $ExtraCount++
    }
}

Write-Host "`n[MANIFEST-UPDATE][SUMMARY] mode=$Mode changed=$ChangedCount missing=$MissingCount extra=$ExtraCount invalid=$InvalidCount unchanged=$UnchangedCount"

if ($Apply) {
    if ($ChangedCount -eq 0 -and $MissingCount -eq 0) {
        Write-Host "[MANIFEST-UPDATE] No changes to apply." -ForegroundColor Green
    } else {
        Write-Host "[MANIFEST-UPDATE] Applying changes to $ManifestFile..." -ForegroundColor Yellow
        $JoinedContent = $NewContent -join [System.Environment]::NewLine
        $JoinedContent | Out-File -FilePath $ManifestFile -Encoding utf8 -NoNewline
        Write-Host "[MANIFEST-UPDATE] Apply complete. Please verify with verify_delivery_manifest.ps1." -ForegroundColor Green
    }
} else {
    Write-Host "[MANIFEST-UPDATE] Dry-run complete. No files were modified." -ForegroundColor Cyan
}

exit 0
