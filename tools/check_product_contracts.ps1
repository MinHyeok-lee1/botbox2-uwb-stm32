# tools/check_product_contracts.ps1
# Product Documentation Contract Drift Guard (External-Facing Version)

$ErrorActionPreference = "Stop"
$Global:Failed = $false

function Write-Result ($Message, $Success) {
    if ($Success) {
        Write-Host "[PASS] $Message" -ForegroundColor Green
    } else {
        Write-Host "[FAIL] $Message" -ForegroundColor Red
        $Global:Failed = $true
    }
}

function Check-File-Content ($FilePath, $Keywords) {
    if (-not (Test-Path $FilePath)) {
        Write-Result "File exists: $FilePath" $false
        return
    }
    
    $content = Get-Content $FilePath -Raw
    $allFound = $true
    foreach ($kw in $Keywords) {
        if ($content -notmatch [regex]::Escape($kw)) {
            Write-Result "Content check ($FilePath): Keyword '$kw' not found" $false
            $allFound = $false
        }
    }
    
    if ($allFound) {
        Write-Result "Content check ($FilePath): All keywords found" $true
    }
}

Write-Host "--- Starting Product Documentation Contract Check ---" -Cyan

# 1. Check essential product file existence
$requiredFiles = @(
    "docs/contracts/UWB_CAN_PACKET_CONTRACT.md",
    "docs/PRODUCT_CAN_INTERFACE.md",
    "docs/PRODUCT_OVERVIEW.md",
    "docs/PRODUCT_QUICK_START.md",
    "docs/PRODUCT_PROVISIONING_GUIDE.md"
)

foreach ($f in $requiredFiles) {
    Write-Result "File existence: $f" (Test-Path $f)
}

# 2. Check CAN Contract content
$canKeywords = @(
    "0x201", "0x202", "0x203",
    "union UwbPacket", "0xAA",
    "int16_t", "checksum",
    "SS-TWR", "DS-TWR",
    "millimeters", "mm"
)
Check-File-Content "docs/contracts/UWB_CAN_PACKET_CONTRACT.md" $canKeywords

# 3. Check Product CAN Interface summary
Check-File-Content "docs/PRODUCT_CAN_INTERFACE.md" $canKeywords

# 4. Check Provisioning/Build guidance keywords
$opKeywords = @(
    "provision.ps1", "firmware.bin", "cfg.bin", "cmake"
)
Check-File-Content "docs/PRODUCT_QUICK_START.md" $opKeywords
Check-File-Content "docs/PRODUCT_PROVISIONING_GUIDE.md" $opKeywords

Write-Host "--- Check Completed ---" -Cyan

if ($Global:Failed) {
    Write-Host "RESULT: FAIL" -ForegroundColor Red
    exit 1
} else {
    Write-Host "RESULT: PASS" -ForegroundColor Green
    exit 0
}
