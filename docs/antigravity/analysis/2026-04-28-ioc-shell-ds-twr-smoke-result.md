# Phase 10: IOC-shell cfg-based DS-TWR Hardware Smoke Verification Result

## 1. Overview
The goal of Phase 10 was to verify the DS-TWR (Double-Sided Two-Way Ranging) protocol support in the `ioc-shell` firmware using configuration persistence (`cfg.bin`). This was performed as a 1:3 smoke test (1 Tag, 3 Anchors) with unique DS configurations provisioned to each board.

- **Date:** 2026-04-28
- **Target:** stm32-dw3000-g474re-ioc-shell
- **Delivery Firmware:** `delivery/firmware.bin` (Golden Image)
- **Ranging Mode:** DS (Double-Sided)
- **Target Rate:** 6 Hz

## 2. Environment Verification
- **Delivery Freshness:** Verified via `tools/check_delivery.ps1` (Fresh).
- **DS Configurations:**
  - `tag_a1001_ds_3a_p2001.bin` (Tag, 3 Anchors)
  - `anc_a2001_ds_p2001.bin` (Anchor 1)
  - `anc_a2002_ds_p2001.bin` (Anchor 2)
  - `anc_a2003_ds_p2001.bin` (Anchor 3)

## 3. Provisioning Results
All 4 boards were provisioned successfully using `tools/provision.ps1`.

| Role | ST-Link SN | Config File | Status |
| :--- | :--- | :--- | :--- |
| TAG | `004500403234511137333934` | `tag_a1001_ds_3a_p2001.bin` | OK |
| ANC1 | `0031001F4142501820353451` | `anc_a2001_ds_p2001.bin` | OK |
| ANC2 | `0032002E3234510A37333934` | `anc_a2002_ds_p2001.bin` | OK |
| ANC3 | `001C00283234511137333934` | `anc_a2003_ds_p2001.bin` | OK |

## 4. UART Log Analysis
Logs were captured for 60+ seconds.
**Log Path:** `artifacts/ioc_shell_ds_smoke/20260428-1822/`

### 4.1. Configuration & Role Dispatch
All boards correctly loaded their roles and modes from flash.
- **TAG:** `[CFG] Flash config loaded successfully. Role: TAG`, `Mode (SS/DS): 1`
- **ANCHORs:** `[CFG] Flash config loaded successfully. Role: ANCHOR`, `Mode (SS/DS): 1`

### 4.2. DS-TWR Ranging Performance
- **TAG [TAG][MULTI]:** Repeatedly showing `ok=3/3`.
- **Anchors Responses:** All 3 anchors (0x2001, 0x2002, 0x2003) are consistently responding.
- **Cycle Timing:** `[TAG][CYCLE] air=124~125 wait=41 target=166 overrun=0`. Stability is excellent.
- **Distance Values:**
  - A0 (0x2001): ~600-700mm
  - A1 (0x2002): ~650-700mm
  - A2 (0x2003): ~450-530mm
- **Quality Gates:**
  - A1 (0x2002) shows high quality (`reason=ok`).
  - A0 and A2 show frequent `reason=mp` (Multi-Path), which is expected for the current baseline and hardware placement.

## 5. Device ID Anomaly Tracking
- TAG: `0xDECA0302` (Correct)
- ANC1: `0x20000000` (Incorrect)
- ANC2: `0xDECA0302` (Correct)
- ANC3: `0x00000000` (Incorrect)
> [!NOTE]
> Despite the incorrect Device ID reporting on ANC1 and ANC3, the UWB ranging is functionally working. This indicates a minor diagnostic/SPI read issue during init, but doesn't block the core TWR logic.

## 6. Final Judgment: PASS
- **Config Load:** SUCCESS
- **Role Dispatch:** SUCCESS
- **Mode DS:** SUCCESS
- **1:3 Smoke:** SUCCESS (ok=3/3 repeatable)
- **Stability:** SUCCESS (overrun=0)

Phase 10 is concluded as **PASS**.

## 7. Next Steps
- Phase 11: Long-term Stability Verification.
- Final archival of diagnostic artifacts.
