# Phase 11: IOC-shell DS-TWR 1:3 Burn-in Result (10 Minutes)

## 1. Overview
The goal of Phase 11 was to verify the long-term stability of the `ioc-shell` firmware in a DS-TWR 1:3 configuration. A 10-minute burn-in test was performed to monitor for protocol failures, cycle overruns, or system crashes.

- **Date:** 2026-04-29
- **Target:** stm32-dw3000-g474re-ioc-shell
- **Delivery Firmware:** `delivery/firmware.bin` (Freshly verified)
- **Protocol:** DS-TWR 1:3
- **Duration:** 600 seconds (10 minutes)

## 2. Environment & Provisioning
All 4 boards were provisioned with the latest golden firmware and DS-specific configurations.

| Role | ST-Link SN | Config File | Status |
| :--- | :--- | :--- | :--- |
| TAG | `004500403234511137333934` | `tag_a1001_ds_3a_p2001.bin` | SUCCESS |
| ANC1 | `0031001F4142501820353451` | `anc_a2001_ds_p2001.bin` | SUCCESS |
| ANC2 | `0032002E3234510A37333934` | `anc_a2002_ds_p2001.bin` | SUCCESS |
| ANC3 | `001C00283234511137333934` | `anc_a2003_ds_p2001.bin` | SUCCESS |

## 3. UART Log Path
**Log Directory:** `artifacts/ioc_shell_ds_burnin/20260429-0920/`

## 4. TAG Stability Summary (10 min)
Analysis of `tag_com18.log`:

| Metric | Count | Observation |
| :--- | :--- | :--- |
| **Total [TAG][MULTI]** | 3502 | ~5.84 Hz (Target 6 Hz baseline) |
| **ok=3/3** | 3499 | **99.91% Success Rate** |
| **ok=2/3** | 0 | - |
| **ok=1/3** | 1 | Single transient drop |
| **ok=0/3** | 2 | Single transient drops |
| **Total [TAG][CYCLE]** | 3502 | Matches multi count |
| **overrun=0** | 3502 | **100% Timing Stability** |
| **overrun=1** | 0 | No task slippage |
| **Errors (Reset/Fault)**| 0 | No system crashes |

## 5. ANCHOR Stability Summary
Analysis of `anc1_com19.log`, `anc2_com15.log`, `anc3_com20.log`:
- **Role Dispatch:** All anchors correctly initialized as ANCHOR in DS mode.
- **Protocol Integrity:**
  - `bad_pan`: 0 (Non-zero events: 0)
  - `bad_dst`: 0
  - `tx_late`: 0
- **Uptime:** No resets or hard faults observed during the 10-minute window.

## 6. Final Judgment: PASS
The system demonstrated exceptional stability under DS-TWR 1:3 operation. With a >99.9% ranging success rate and zero cycle overruns, the `ioc-shell` migration is considered robust for the 6Hz baseline.

## 7. Observed Anomalies
- **Minor Packet Loss:** Only 3 ranging events out of 3502 failed to receive all 3 anchor responses. This is well within expected RF interference thresholds for a smoke/burn-in test.
- **Device ID:** ANC1 and ANC3 continue to report anomalous IDs (`0x20000000` / `0x00000000`), but as confirmed in previous phases, this does not affect functional ranging.

## 8. Next Action
- **Phase 11B:** Extended 30-minute burn-in to confirm no long-tail drift or heat-related issues.
- **Merge Preparation:** Final documentation review before merging to `main`.
