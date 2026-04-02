<#
.SYNOPSIS
    STM32-DW3000 UWB Device Provisioning Tool (Windows PowerShell)

.DESCRIPTION
    This script safely provisions firmware (.bin) and configuration (.bin)
    to the board using STM32CubeProgrammer CLI.

.PARAMETER Firmware
    Path to firmware binary (.bin). (Alias: -fw)

.PARAMETER Config
    Path to configuration binary (.bin). (Alias: -cfg)

.PARAMETER SerialNumber
    ST-Link Serial Number. If omitted, auto-detects if exactly one probe is connected. (Alias: -sn)

.PARAMETER Frequency
    SWD Frequency (kHz). Default is 4000. (Alias: -freq)

.PARAMETER DryRun
    Prints the CLI commands that would be executed without actually writing.

.EXAMPLE
    .\delivery\tools\provision.ps1 -fw .\delivery\firmware.bin -cfg .\delivery\configs\tag_default.bin
    Auto-detects a single probe and provisions both.

.EXAMPLE
    .\delivery\tools\provision.ps1 -cfg .\delivery\configs\anchor_default.bin -sn 004D003D3234
    Updates config only for the specified probe.
#>

[CmdletBinding()]
param (
    [Alias("fw")]
    [string]$Firmware,

    [Alias("cfg")]
    [string]$Config,

    [Alias("sn")]
    [string]$SerialNumber,

    [Alias("freq")]
    [int]$Frequency = 4000,

    [switch]$DryRun
)

# Constants
$FW_FLASH_ADDR = "0x08000000"
$CFG_FLASH_ADDR = "0x0807F800"
$MAX_FW_SIZE = 510 * 1024   # 522240 bytes
$EXPECTED_CFG_SIZE = 40

# Logging
function Write-Log {
    param([string]$Message, [ConsoleColor]$Color = "White")
    Write-Host "==> $Message" -ForegroundColor $Color
}

function Write-ErrorLog {
    param([string]$Message)
    Write-Host "[ERR] $Message" -ForegroundColor Red
}

# 1. CubeProgrammer CLI Discovery
function Resolve-Stm32CliPath {
    $candidates = @()

    # Current process env
    if ($env:STM32_CLI) {
        $candidates += $env:STM32_CLI
    }

    # User / Machine persistent env
    $userCli = [Environment]::GetEnvironmentVariable("STM32_CLI", "User")
    if ($userCli) {
        $candidates += $userCli
    }

    $machineCli = [Environment]::GetEnvironmentVariable("STM32_CLI", "Machine")
    if ($machineCli) {
        $candidates += $machineCli
    }

    # Common install paths
    $candidates += @(
        "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe",
        "C:\Program Files\STMicroelectronics\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe",
        "C:\Program Files (x86)\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe",
        "C:\Program Files (x86)\STMicroelectronics\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
    )

    foreach ($candidate in ($candidates | Select-Object -Unique)) {
        if ($candidate -and (Test-Path $candidate)) {
            return (Resolve-Path $candidate).Path
        }
    }

    $cmd = Get-Command "STM32_Programmer_CLI.exe" -ErrorAction SilentlyContinue
    if ($cmd -and $cmd.Source -and (Test-Path $cmd.Source)) {
        return $cmd.Source
    }

    return $null
}

$CliPath = Resolve-Stm32CliPath

if (-not $CliPath) {
    Write-ErrorLog "STM32CubeProgrammer not found."
    Write-Log "Checked: process/user/machine STM32_CLI, PATH, and common install locations." "Yellow"
    Write-Log "Set STM32_CLI to the full path of STM32_Programmer_CLI.exe if needed." "Yellow"
    exit 2
}

Write-Log "STM32 CLI : $CliPath" "DarkCyan"

# 2. Argument Validation
if (-not $Firmware -and -not $Config) {
    Write-ErrorLog "Specify at least -Firmware or -Config."
    exit 1
}

# 3. File Validation
if ($Firmware) {
    if (-not (Test-Path $Firmware)) {
        Write-ErrorLog "Firmware file not found: $Firmware"
        exit 4
    }

    $fwItem = Get-Item $Firmware
    $Firmware = $fwItem.FullName
    $fwSize = $fwItem.Length

    if ($fwSize -eq 0) {
        Write-ErrorLog "Firmware file is empty (0 bytes)."
        exit 4
    }

    if ($fwSize -gt $MAX_FW_SIZE) {
        Write-ErrorLog "Firmware size ($fwSize bytes) exceeds 510K limit. Aborting."
        exit 4
    }

    Write-Log "Firmware : $Firmware ($fwSize bytes)" "Cyan"
}

if ($Config) {
    if (-not (Test-Path $Config)) {
        Write-ErrorLog "Config file not found: $Config"
        exit 5
    }

    $cfgItem = Get-Item $Config
    $Config = $cfgItem.FullName
    $cfgSize = $cfgItem.Length

    if ($cfgSize -ne $EXPECTED_CFG_SIZE) {
        Write-ErrorLog "Config size ($cfgSize bytes) != expected $EXPECTED_CFG_SIZE bytes."
        exit 5
    }

    Write-Log "Config   : $Config ($cfgSize bytes)" "Cyan"
}

# 4. Probe Discovery
if (-not $SerialNumber) {
    Write-Log "Auto-detecting ST-Link probe..."

    $listOutput = & $CliPath -l 2>&1
    $probeMatches = @($listOutput | Select-String -Pattern "ST-LINK SN\s*:\s*([A-F0-9]+)")
    $uniqueProbes = @($probeMatches | ForEach-Object { $_.Matches.Groups[1].Value } | Select-Object -Unique)

    if ($uniqueProbes.Count -eq 0) {
        Write-ErrorLog "No ST-Link probe found."
        exit 3
    }
    elseif ($uniqueProbes.Count -gt 1) {
        Write-ErrorLog "Multiple probes found. Please specify -sn <Serial>."
        Write-Host ($uniqueProbes -join "`n")
        exit 3
    }
    else {
        $SerialNumber = $uniqueProbes[0]
        Write-Log "Auto-selected SN: $SerialNumber" "Green"
    }
}
else {
    Write-Log "Specified ST-Link SN: $SerialNumber"
}

$connArgs = @("port=SWD", "freq=$Frequency", "sn=$SerialNumber")

# 5. Execution Helper
function Run-ST-CLI {
    param([string[]]$Arguments)

    $global:LASTEXITCODE = 0
    $allArgs = @("-c") + $connArgs + $Arguments

    if ($DryRun) {
        Write-Log "[DryRun] & `"$CliPath`" $($allArgs -join ' ')" "Yellow"
        return
    }

    & $CliPath $allArgs
}

# Step 1: Write Firmware
if ($Firmware) {
    Write-Log "Writing firmware to $FW_FLASH_ADDR ..."
    Run-ST-CLI @("-w", $Firmware, $FW_FLASH_ADDR, "-v")
    if ($LASTEXITCODE -ne 0) {
        Write-ErrorLog "Firmware write failed (Exit Code: $LASTEXITCODE)"
        exit 6
    }
}

# Step 2: Write Config
if ($Config) {
    Write-Log "Writing config to $CFG_FLASH_ADDR ..."
    Run-ST-CLI @("-w", $Config, $CFG_FLASH_ADDR, "-v")
    if ($LASTEXITCODE -ne 0) {
        Write-ErrorLog "Config write failed (Exit Code: $LASTEXITCODE)"
        exit 6
    }
}

# Step 3: Reset
Write-Log "Resetting MCU..."
Run-ST-CLI @("-rst")
if ($LASTEXITCODE -ne 0) {
    Write-ErrorLog "Reset failed (Exit Code: $LASTEXITCODE)"
    exit 7
}

Write-Log "Provisioning done" "Green"
exit 0