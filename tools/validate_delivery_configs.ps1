#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Read-only delivery config preset validator.
.DESCRIPTION
    Validates required delivery/configs/*.bin presets using the packed uwb_cfg_t
    layout from Core/Inc/uwb_cfg.h and the software CRC32 flow from
    Core/Src/uwb_cfg.c. This script never modifies cfg binaries.
#>

$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$ConfigDir = Join-Path $Root "delivery\configs"

$script:OkCount = 0
$script:FailCount = 0

function Write-Ok {
    param([string]$Message)
    Write-Host "[OK] $Message" -ForegroundColor Green
    $script:OkCount++
}

function Write-Fail {
    param([string]$Message)
    Write-Host "[FAIL] $Message" -ForegroundColor Red
    $script:FailCount++
}

function Get-U16Le {
    param([byte[]]$Bytes, [int]$Offset)
    return [uint16]([int]$Bytes[$Offset] -bor ([int]$Bytes[$Offset + 1] -shl 8))
}

function Get-U32Le {
    param([byte[]]$Bytes, [int]$Offset)
    [uint64]$Value = [uint64]$Bytes[$Offset]
    $Value = $Value -bor ([uint64]$Bytes[$Offset + 1] -shl 8)
    $Value = $Value -bor ([uint64]$Bytes[$Offset + 2] -shl 16)
    $Value = $Value -bor ([uint64]$Bytes[$Offset + 3] -shl 24)
    return [uint32]$Value
}

function Get-UwbCfgCrc32 {
    param([byte[]]$Bytes, [int]$Length)

    [uint64]$Crc = 4294967295
    for ($i = 0; $i -lt $Length; $i++) {
        $Crc = ($Crc -bxor ([uint64]$Bytes[$i] -shl 24)) -band 4294967295
        for ($j = 0; $j -lt 8; $j++) {
            if (($Crc -band 2147483648) -ne 0) {
                $Crc = (($Crc -shl 1) -bxor 79764919) -band 4294967295
            } else {
                $Crc = ($Crc -shl 1) -band 4294967295
            }
        }
    }
    return [uint32]$Crc
}

function Test-Equal {
    param(
        [string]$Label,
        $Actual,
        $Expected,
        [string]$Format = ""
    )

    if ($Actual -eq $Expected) {
        if ($Format -eq "hex16") {
            Write-Ok ("{0}: 0x{1:X4}" -f $Label, [uint16]$Actual)
        } elseif ($Format -eq "hex32") {
            Write-Ok ("{0}: 0x{1:X8}" -f $Label, [uint32]$Actual)
        } else {
            Write-Ok ("{0}: {1}" -f $Label, $Actual)
        }
    } else {
        if ($Format -eq "hex16") {
            Write-Fail ("{0}: got 0x{1:X4}, expected 0x{2:X4}" -f $Label, [uint16]$Actual, [uint16]$Expected)
        } elseif ($Format -eq "hex32") {
            Write-Fail ("{0}: got 0x{1:X8}, expected 0x{2:X8}" -f $Label, [uint32]$Actual, [uint32]$Expected)
        } else {
            Write-Fail ("{0}: got {1}, expected {2}" -f $Label, $Actual, $Expected)
        }
    }
}

function Read-UwbCfg {
    param([string]$Path)

    [byte[]]$Bytes = Get-Content -Path $Path -Encoding Byte
    $Anchors = @()
    for ($i = 0; $i -lt 8; $i++) {
        $Anchors += Get-U16Le $Bytes (20 + ($i * 2))
    }

    return [pscustomobject]@{
        Bytes = $Bytes
        Magic = Get-U32Le $Bytes 0
        Version = Get-U16Le $Bytes 4
        Length = Get-U16Le $Bytes 6
        Role = [byte]$Bytes[8]
        RangingMode = [byte]$Bytes[9]
        PanId = Get-U16Le $Bytes 10
        ShortAddr = Get-U16Le $Bytes 12
        AnchorCount = [byte]$Bytes[14]
        FilterProfile = [byte]$Bytes[15]
        CommMode = [byte]$Bytes[16]
        AnchorIds = $Anchors
        Crc32 = Get-U32Le $Bytes 36
        CalcCrc32 = Get-UwbCfgCrc32 $Bytes 36
    }
}

function Test-ConfigPreset {
    param(
        [string]$FileName,
        [byte]$ExpectedRole,
        [uint16]$ExpectedShortAddr,
        [uint16]$ExpectedPanId,
        [object]$ExpectedAnchorCount,
        [uint16[]]$ExpectedAnchors
    )

    Write-Host ""
    Write-Host "--- $FileName ---" -ForegroundColor Cyan
    $Path = Join-Path $ConfigDir $FileName

    if (-not (Test-Path $Path)) {
        Write-Fail "exists: $FileName"
        return
    }
    Write-Ok "exists: $FileName"

    $Item = Get-Item $Path
    Test-Equal "size bytes" $Item.Length 40
    if ($Item.Length -ne 40) {
        return
    }

    $Cfg = Read-UwbCfg $Path

    Test-Equal "magic" $Cfg.Magic 0x55574243 "hex32"
    Test-Equal "version" $Cfg.Version 1
    Test-Equal "length" $Cfg.Length 40
    Test-Equal "role" $Cfg.Role $ExpectedRole
    Test-Equal "ranging mode DS" $Cfg.RangingMode 1
    Test-Equal "PAN" $Cfg.PanId $ExpectedPanId "hex16"
    Test-Equal "short addr" $Cfg.ShortAddr $ExpectedShortAddr "hex16"
    Test-Equal "filter profile" $Cfg.FilterProfile 1
    Test-Equal "comm mode" $Cfg.CommMode 0
    Test-Equal "crc32" $Cfg.Crc32 $Cfg.CalcCrc32 "hex32"

    if ($null -ne $ExpectedAnchorCount) {
        Test-Equal "anchor count" $Cfg.AnchorCount ([byte]$ExpectedAnchorCount)
    }

    if (($null -ne $ExpectedAnchors) -and ($ExpectedAnchors.Count -gt 0)) {
        for ($i = 0; $i -lt $ExpectedAnchors.Count; $i++) {
            Test-Equal ("anchor_ids[{0}]" -f $i) $Cfg.AnchorIds[$i] $ExpectedAnchors[$i] "hex16"
        }
    }
}

Write-Host "=== Delivery Config Preset Validator ===" -ForegroundColor Cyan
Write-Host "root=$Root"
Write-Host "config_dir=$ConfigDir"

if (-not (Test-Path $ConfigDir)) {
    Write-Fail "config directory exists: $ConfigDir"
} else {
    Write-Ok "config directory exists"
}

Test-ConfigPreset `
    -FileName "tag_a1001_ds_3a_p2001.bin" `
    -ExpectedRole 1 `
    -ExpectedShortAddr 0x1001 `
    -ExpectedPanId 0x2001 `
    -ExpectedAnchorCount 3 `
    -ExpectedAnchors @(0x2001, 0x2002, 0x2003)

Test-ConfigPreset `
    -FileName "anc_a2001_ds_p2001.bin" `
    -ExpectedRole 0 `
    -ExpectedShortAddr 0x2001 `
    -ExpectedPanId 0x2001 `
    -ExpectedAnchorCount $null `
    -ExpectedAnchors @()

Test-ConfigPreset `
    -FileName "anc_a2002_ds_p2001.bin" `
    -ExpectedRole 0 `
    -ExpectedShortAddr 0x2002 `
    -ExpectedPanId 0x2001 `
    -ExpectedAnchorCount $null `
    -ExpectedAnchors @()

Test-ConfigPreset `
    -FileName "anc_a2003_ds_p2001.bin" `
    -ExpectedRole 0 `
    -ExpectedShortAddr 0x2003 `
    -ExpectedPanId 0x2001 `
    -ExpectedAnchorCount $null `
    -ExpectedAnchors @()

Write-Host ""
Write-Host "--- Summary ---" -ForegroundColor Cyan
Write-Host "ok=$script:OkCount fail=$script:FailCount"

if ($script:FailCount -ne 0) {
    exit 1
}

exit 0
