param(
    [Parameter(Position = 0)]
    [ValidateSet("help", "status", "build", "gate", "list-stlink", "flash", "flash-all-ds", "flash-all-ss", "experiment-build-stop1", "export-customer", "hash-delivery", "capture-log")]
    [string]$Command = "help",

    [ValidateSet("evb", "product", "customer")]
    [string]$Repo = "product",

    [string]$Preset = "Release",

    [ValidateSet("tag", "anc1", "anc2", "anc3")]
    [string]$Role,

    [ValidateSet("ds", "ss")]
    [string]$Mode = "ds",

    [string]$Sn,
    [string]$TagSn,
    [string]$Anc1Sn,
    [string]$Anc2Sn,
    [string]$Anc3Sn,

    [string]$Port,
    [string]$Name,
    [int]$DurationSec = 60,
    [int]$Baud = 115200,
    [string]$OutDir,

    [switch]$Apply,
    [switch]$DryRun,
    [switch]$Help
)

$ErrorActionPreference = "Stop"

$WorkspaceRoot = "C:\Users\ryan\dev\embedded\UWB_STM32"
$Repos = @{
    evb      = "01_stm32-dw3000-g474re-evb"
    product  = "02_stm32-dw3000-g474re"
    customer = "03_stm32-dw3000-G474RE-customer"
}

$CfgMap = @{
    ds = @{
        tag  = "delivery\configs\tag_a1001_ds_3a_p2001.bin"
        anc1 = "delivery\configs\anc_a2001_ds_p2001.bin"
        anc2 = "delivery\configs\anc_a2002_ds_p2001.bin"
        anc3 = "delivery\configs\anc_a2003_ds_p2001.bin"
    }
    ss = @{
        tag  = "delivery\configs\tag_a1001_ss_3a_p2001.bin"
        anc1 = "delivery\configs\anc_a2001_ss_p2001.bin"
        anc2 = "delivery\configs\anc_a2002_ss_p2001.bin"
        anc3 = "delivery\configs\anc_a2003_ss_p2001.bin"
    }
}

function Show-Help {
    Write-Host "UWB STM32 operation helper"
    Write-Host ""
    Write-Host "Examples:"
    Write-Host "  powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 status -Repo product"
    Write-Host "  powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 build -Repo product -Preset Release"
    Write-Host "  powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 gate -Repo product"
    Write-Host "  powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 list-stlink"
    Write-Host "  powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 flash -Repo product -Role anc1 -Mode ds -Sn 0032002E3234510A37333934"
    Write-Host "  powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 flash-all-ds -Repo product -Anc1Sn <SN> -Anc2Sn <SN> -Anc3Sn <SN> -TagSn <SN>"
    Write-Host "  powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 experiment-build-stop1 -Repo product"
    Write-Host "  powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 export-customer"
    Write-Host "  powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 export-customer -Apply"
    Write-Host "  powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 hash-delivery -Repo product"
    Write-Host "  powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 capture-log -Repo product -Port COM18 -Name anchor1 -DurationSec 60"
    Write-Host "  powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 status -Repo customer"
    Write-Host "  powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 hash-delivery -Repo customer"
    Write-Host "  powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 capture-log -Repo customer -Port COM18 -Name anchor1 -DurationSec 60"
    Write-Host ""
    Write-Host "Safety:"
    Write-Host "  flash and flash-all run immediately only when all required SNs are provided."
    Write-Host "  export-customer prints the official command by default; use -Apply to run it."
    Write-Host "  experiment-build-stop1 uses build\stop1-experiment and PRODUCT_COPY_DELIVERY_ARTIFACTS=OFF."
    Write-Host "  capture-log writes under artifacts by default and requires -Port and -Name."
}

function Get-RepoPath([string]$Name) {
    if (-not $Repos.ContainsKey($Name)) {
        throw "Unknown repo: $Name"
    }
    $path = Join-Path $WorkspaceRoot $Repos[$Name]
    if (-not (Test-Path $path)) {
        throw "Repo path not found: $path"
    }
    return $path
}

function Invoke-InRepo([string]$Name, [scriptblock]$Block) {
    $path = Get-RepoPath $Name
    Push-Location $path
    try {
        & $Block
    } finally {
        Pop-Location
    }
}

function Show-Command([string]$Text) {
    Write-Host $Text -ForegroundColor Cyan
}

function Get-FlashCfg([string]$ModeName, [string]$RoleName) {
    if (-not $CfgMap.ContainsKey($ModeName)) {
        throw "Unknown mode: $ModeName"
    }
    if (-not $CfgMap[$ModeName].ContainsKey($RoleName)) {
        throw "Unknown role: $RoleName"
    }
    return $CfgMap[$ModeName][$RoleName]
}

function Invoke-FlashOne([string]$RepoName, [string]$RoleName, [string]$ModeName, [string]$Serial) {
    if ([string]::IsNullOrWhiteSpace($Serial)) {
        throw "Missing -Sn for flash."
    }
    $cfg = Get-FlashCfg $ModeName $RoleName
    Invoke-InRepo $RepoName {
        if (-not (Test-Path ".\tools\provision.ps1")) {
            throw "Missing tools\provision.ps1 in repo: $RepoName"
        }
        if (-not (Test-Path ".\delivery\firmware.bin")) {
            throw "Missing delivery\firmware.bin in repo: $RepoName"
        }
        if (-not (Test-Path $cfg)) {
            throw "Missing cfg: $cfg"
        }
        $cmd = "powershell -ExecutionPolicy Bypass -File .\tools\provision.ps1 -sn $Serial -fw .\delivery\firmware.bin -cfg .\$cfg"
        Show-Command $cmd
        if (-not $DryRun) {
            powershell -ExecutionPolicy Bypass -File ".\tools\provision.ps1" -sn $Serial -fw ".\delivery\firmware.bin" -cfg ".\$cfg"
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
    }
}

function Invoke-FlashAll([string]$RepoName, [string]$ModeName) {
    $pairs = @(
        @{ Role = "anc1"; Sn = $Anc1Sn },
        @{ Role = "anc2"; Sn = $Anc2Sn },
        @{ Role = "anc3"; Sn = $Anc3Sn },
        @{ Role = "tag";  Sn = $TagSn }
    )

    foreach ($p in $pairs) {
        if ([string]::IsNullOrWhiteSpace($p.Sn)) {
            Write-Host "Missing SN for $($p.Role). Showing copy-ready commands with <SN> placeholders." -ForegroundColor Yellow
            Invoke-InRepo $RepoName {
                foreach ($q in $pairs) {
                    $cfg = Get-FlashCfg $ModeName $q.Role
                    Show-Command "powershell -ExecutionPolicy Bypass -File .\tools\provision.ps1 -sn <SN> -fw .\delivery\firmware.bin -cfg .\$cfg"
                }
            }
            return
        }
    }

    foreach ($p in $pairs) {
        Invoke-FlashOne $RepoName $p.Role $ModeName $p.Sn
    }
}

function Invoke-CaptureLog([string]$RepoName, [string]$ComPort, [string]$LogName, [int]$CaptureBaud, [int]$Seconds, [string]$OutputDir) {
    if ([string]::IsNullOrWhiteSpace($ComPort)) {
        throw "capture-log requires -Port, for example -Port COM18."
    }
    if ([string]::IsNullOrWhiteSpace($LogName)) {
        throw "capture-log requires -Name, for example -Name anchor1."
    }
    if ($Seconds -le 0) {
        throw "capture-log requires -DurationSec greater than 0."
    }
    if ($CaptureBaud -le 0) {
        throw "capture-log requires -Baud greater than 0."
    }

    Invoke-InRepo $RepoName {
        $safeName = $LogName -replace '[^A-Za-z0-9_.-]', '_'
        $safePort = $ComPort -replace '[^A-Za-z0-9_.-]', '_'
        $stampDir = Get-Date -Format "yyyyMMdd-HHmm"
        $stampFile = Get-Date -Format "yyyyMMdd-HHmmss"

        if ([string]::IsNullOrWhiteSpace($OutputDir)) {
            $targetDir = Join-Path "artifacts\operator_delivery" $stampDir
        } else {
            $targetDir = $OutputDir
        }

        if (-not (Test-Path $targetDir)) {
            New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
        }

        $outFile = Join-Path $targetDir ("{0}_{1}_{2}.log" -f $safeName, $safePort, $stampFile)
        $fullOutFile = (Resolve-Path $targetDir).Path
        $fullOutFile = Join-Path $fullOutFile (Split-Path $outFile -Leaf)

        Write-Host "=== UART capture ==="
        Write-Host "Port        : $ComPort"
        Write-Host "Baud        : $CaptureBaud"
        Write-Host "DurationSec : $Seconds"
        Write-Host "Output      : $fullOutFile"

        $serial = $null
        $writer = $null
        try {
            $serial = New-Object System.IO.Ports.SerialPort $ComPort, $CaptureBaud, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
            $serial.ReadTimeout = 500
            $serial.NewLine = "`n"
            $serial.Open()

            $utf8NoBom = New-Object System.Text.UTF8Encoding $false
            $writer = New-Object System.IO.StreamWriter $fullOutFile, $false, $utf8NoBom
            $deadline = (Get-Date).AddSeconds($Seconds)

            while ((Get-Date) -lt $deadline) {
                try {
                    $chunk = $serial.ReadExisting()
                    if ($chunk.Length -gt 0) {
                        $writer.Write($chunk)
                        $writer.Flush()
                        Write-Host $chunk -NoNewline
                    } else {
                        Start-Sleep -Milliseconds 50
                    }
                } catch [System.TimeoutException] {
                    Start-Sleep -Milliseconds 50
                }
            }
        } catch {
            Write-Error ("[CAPTURE][ERR] {0}" -f $_.Exception.Message)
            exit 1
        } finally {
            if ($writer -ne $null) {
                $writer.Flush()
                $writer.Close()
            }
            if ($serial -ne $null) {
                if ($serial.IsOpen) {
                    $serial.Close()
                }
                $serial.Dispose()
            }
            Write-Host ""
            Write-Host "Capture closed: $fullOutFile"
        }
    }
}

if ($Help -or $Command -eq "help") {
    Show-Help
    exit 0
}

switch ($Command) {
    "status" {
        Invoke-InRepo $Repo {
            git branch --show-current
            git status --short
            git log -n 5 --oneline
            git diff --stat
        }
    }
    "build" {
        if ($Repo -ne "product" -and $Repo -ne "customer") {
            throw "build is supported for product/customer only."
        }
        Invoke-InRepo $Repo {
            Write-Host "WARNING: Release builds can refresh delivery\firmware.bin and delivery\firmware.hex when PRODUCT_COPY_DELIVERY_ARTIFACTS is ON." -ForegroundColor Yellow
            cmake --preset $Preset
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
            cmake --build "build\$Preset" --parallel 8
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
    }
    "gate" {
        if ($Repo -ne "product") {
            throw "gate is supported from product repo only."
        }
        Invoke-InRepo "product" {
            powershell -ExecutionPolicy Bypass -File ".\tools\run_release_acceptance_gates.ps1"
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
    }
    "list-stlink" {
        $cli = "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
        if (-not (Test-Path $cli)) {
            throw "STM32_Programmer_CLI.exe not found: $cli"
        }
        & $cli --list
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    "flash" {
        if ([string]::IsNullOrWhiteSpace($Role)) {
            throw "flash requires -Role tag|anc1|anc2|anc3."
        }
        Invoke-FlashOne $Repo $Role $Mode $Sn
    }
    "flash-all-ds" {
        Invoke-FlashAll $Repo "ds"
    }
    "flash-all-ss" {
        Invoke-FlashAll $Repo "ss"
    }
    "experiment-build-stop1" {
        if ($Repo -ne "product") {
            throw "STOP1 experiment build is Product-only."
        }
        Invoke-InRepo "product" {
            $cmd = 'cmake -S . -B build/stop1-experiment "-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake" "-DCMAKE_BUILD_TYPE=Release" "-DPRODUCT_LOW_POWER_MODE=2" "-DPRODUCT_LOW_POWER_AUTO_ENTRY=1" "-DPRODUCT_PC0_USER_ACTION=1" "-DPRODUCT_UART_DIAG=0" "-DPRODUCT_GPIO_DIAG=0" "-DPRODUCT_DUAL_UART_DIAG=0" "-DPRODUCT_MINIMAL_BOOT_LED_DIAG=0" "-DPRODUCT_UART_SMOKE_KNOWN_GOOD=0" "-DPRODUCT_EXPERIMENTAL_STOP1_SLOW_LISTEN=1" "-DPRODUCT_COPY_DELIVERY_ARTIFACTS=OFF"'
            Show-Command $cmd
            Invoke-Expression $cmd
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
            cmake --build "build\stop1-experiment" --parallel 8
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
    }
    "export-customer" {
        $evbPath = Get-RepoPath "evb"
        $script = Join-Path $evbPath "tools\export_product_to_customer.ps1"
        if (-not (Test-Path $script)) {
            throw "Official export script not found: $script"
        }

        Invoke-InRepo "customer" {
            $dirty = git status --short
            if ($dirty) {
                Write-Host "Customer working tree is dirty. Review before export:" -ForegroundColor Yellow
                $dirty | ForEach-Object { Write-Host $_ }
                if (-not $Apply) {
                    Write-Host "Dry display only. Re-run with -Apply after confirming Customer dirty state is safe." -ForegroundColor Yellow
                }
            }
        }

        Push-Location $evbPath
        try {
            $cmd = "powershell -ExecutionPolicy Bypass -File .\tools\export_product_to_customer.ps1 -Apply -SkipStatusCheck -SkipContractCheck"
            Show-Command $cmd
            if ($Apply) {
                powershell -ExecutionPolicy Bypass -File ".\tools\export_product_to_customer.ps1" -Apply -SkipStatusCheck -SkipContractCheck
                if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
            }
        } finally {
            Pop-Location
        }
    }
    "hash-delivery" {
        Invoke-InRepo $Repo {
            Get-FileHash ".\delivery\firmware.bin" -Algorithm SHA256
            Get-FileHash ".\delivery\firmware.hex" -Algorithm SHA256
        }
    }
    "capture-log" {
        Invoke-CaptureLog $Repo $Port $Name $Baud $DurationSec $OutDir
    }
}
