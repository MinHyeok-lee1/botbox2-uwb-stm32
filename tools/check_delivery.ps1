#!/usr/bin/env pwsh
# tools/check_delivery.ps1
# Simple script to check if explicit delivery firmware assets are present.

$DeliveryDir = Join-Path $PSScriptRoot "..\delivery"

$ExpectedDeliveryAssets = @(
    "firmware_product_ch5.bin",
    "firmware_product_ch9.bin",
    "firmware_rxdiag_ch5.bin",
    "firmware_rxdiag_ch9.bin",
    "firmware_variants.json",
    "manifest.txt"
)

function Test-DeliveryAsset {
    param(
        [string]$DeliveryArtifact
    )

    if (-not (Test-Path $DeliveryArtifact)) {
        Write-Host "[WARNING] Missing delivery asset: $DeliveryArtifact." -ForegroundColor Yellow
        return $false
    }

    Write-Host "[OK] Found delivery asset: $DeliveryArtifact." -ForegroundColor Green
    return $true
}

$AllPresent = $true
foreach ($asset in $ExpectedDeliveryAssets) {
    $path = Join-Path $DeliveryDir $asset
    if (-not (Test-DeliveryAsset -DeliveryArtifact $path)) {
        $AllPresent = $false
    }
}

if ($AllPresent) {
    Write-Host "[OK] delivery firmware assets are present." -ForegroundColor Green
}
else {
    Write-Host "[WARNING] One or more delivery firmware assets are missing." -ForegroundColor Yellow
}
