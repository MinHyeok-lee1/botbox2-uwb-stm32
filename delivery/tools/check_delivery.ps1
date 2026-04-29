#!/usr/bin/env pwsh
# tools/check_delivery.ps1
# Simple script to check if delivery firmware artifacts match the latest build output.

$ProjectName = "stm32-dw3000-g474re-ioc-shell"
$BuildDir = Join-Path $PSScriptRoot "..\..\build\Debug"
$DeliveryDir = Join-Path $PSScriptRoot ".."

$BuildBin = Join-Path $BuildDir "$ProjectName.bin"
$DeliveryBin = Join-Path $DeliveryDir "firmware.bin"
$BuildHex = Join-Path $BuildDir "$ProjectName.hex"
$DeliveryHex = Join-Path $DeliveryDir "firmware.hex"

function Test-DeliveryFreshness {
    param(
        [string]$BuildArtifact,
        [string]$DeliveryArtifact,
        [string]$Label
    )

    if (-not (Test-Path $BuildArtifact)) {
        Write-Host "[SKIP] $Label build artifact not found at $BuildArtifact." -ForegroundColor Gray
        return $true
    }

    if (-not (Test-Path $DeliveryArtifact)) {
        Write-Host "[WARNING] $Label is MISSING at $DeliveryArtifact." -ForegroundColor Yellow
        return $false
    }

    $BuildHash = (Get-FileHash -Path $BuildArtifact).Hash
    $DeliveryHash = (Get-FileHash -Path $DeliveryArtifact).Hash

    if ($BuildHash -eq $DeliveryHash) {
        Write-Host "[OK] $Label is fresh (matches $BuildArtifact)." -ForegroundColor Green
        return $true
    }

    Write-Host "[WARNING] $Label is STALE (mismatch with $BuildArtifact)." -ForegroundColor Yellow
    return $false
}

$BinFresh = Test-DeliveryFreshness -BuildArtifact $BuildBin -DeliveryArtifact $DeliveryBin -Label "delivery/firmware.bin"
$HexFresh = Test-DeliveryFreshness -BuildArtifact $BuildHex -DeliveryArtifact $DeliveryHex -Label "delivery/firmware.hex"

if ($BinFresh -and $HexFresh) {
    Write-Host "[OK] delivery firmware artifacts are fresh." -ForegroundColor Green
} else {
    Write-Host "[WARNING] One or more delivery firmware artifacts need sync." -ForegroundColor Yellow
    Write-Host "Run build again or manually copy the stale/missing artifact to sync."
}
