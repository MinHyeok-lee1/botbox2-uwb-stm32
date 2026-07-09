# Customer Operator Quick Commands

Customer is a delivery/field validation repo. Use these commands for status, flash, UART capture, ST-LINK inventory, and delivery hash checks. Do not hand-edit Customer runtime source or cfg files.

All commands are Windows PowerShell one-line commands. Do not use Bash line continuation.

## Start Here

### 1. Customer ANCHOR1 Flash

Use this for the common field validation flash. Risk: flashes Customer firmware variants (e.g. delivery\firmware_product_ch5.bin) and selected cfg to the target board.

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 customer-flash -Role anc1 -Mode ds -Sn 0032002E3234510A37333934
```

### 2. Customer Hash Check

Use this before or after flashing to record the delivery firmware identity. Risk: read-only.

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 hash-delivery -Repo customer
```

### 3. Customer Log Capture

Use this to save UART logs under `artifacts/` instead of the repo root. Risk: read-only serial capture.

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 capture-log -Repo customer -Port COM18 -Name anchor1 -DurationSec 60
```

## Status

Use in Customer repo. Risk: read-only.

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 status -Repo customer
```

## ST-LINK Inventory

Use in Customer repo or any workspace shell. Risk: read-only.

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 list-stlink
```

## Delivery Hash

Use in Customer repo. Risk: read-only.

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 hash-delivery -Repo customer
```

## ANCHOR1 DS Flash

Use in Customer repo. Risk: flashes board and writes cfg to target flash. Confirm ST-LINK SN before running.

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 customer-flash -Role anc1 -Mode ds -Sn 0032002E3234510A37333934
```

DS cfg mapping:

| Role | Address | cfg |
| --- | --- | --- |
| ANCHOR1 | `0x2001` | `delivery\configs\anc_a2001_ds_p2001.bin` |
| ANCHOR2 | `0x2002` | `delivery\configs\anc_a2002_ds_p2001.bin` |
| ANCHOR3 | `0x2003` | `delivery\configs\anc_a2003_ds_p2001.bin` |
| TAG | `0x1001` | `delivery\configs\tag_a1001_ds_3a_p2001.bin` |

## UART Capture

Use in Customer repo. Risk: read-only serial capture. Requires `-Port` and `-Name`.

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 capture-log -Repo customer -Port COM18 -Name anchor1 -DurationSec 60
powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 capture-log -Repo customer -Port COM15 -Name tag -DurationSec 60
```

Default output:

```text
artifacts/operator_delivery/YYYYMMDD-HHMM/<name>_<port>_<YYYYMMDD-HHMMSS>.log
```

Use a phase-specific directory when needed:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 capture-log -Repo customer -Port COM18 -Name anchor1 -DurationSec 300 -OutDir artifacts/customer_can_validation
```

## Field CAN Check

Expected ANCHOR logs after CAN TX recovery export:

```text
[CAN][INIT] start ok
[CAN][TX] id=0x201 dist=<mm>
[CAN][TX] ok id=0x201 free=<level> err=0x0
[ANC][DS][DIST] AID=0x2001 src=0x1001 dist=<mm> can_id=0x201
```

Mainboard receive confirmation is still a hardware observation and should be recorded in the matching `artifacts/` notes.

## Guardrails

- Do not manually edit Customer `Core/`.
- Do not mutate cfg files.
- Do not refresh delivery firmware during command kit work.
- Product remains the canonical command kit source.
- Customer helper exists for delivery/field validation convenience.
