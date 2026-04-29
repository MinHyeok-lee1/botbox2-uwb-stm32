# Phase 13: Main Branch Post-Merge DS-TWR Smoke Result

## 1. Overview
The goal of Phase 13 was to verify that the UWB ranging functionality remains stable on the `main` branch after the bulk runtime transplant merge. A 60-second post-merge smoke test was performed in a DS-TWR 1:3 configuration.

- **Date:** 2026-04-29
- **Branch:** `main`
- **Commit:** `031ac42`
- **Delivery Firmware:** `delivery/firmware.bin` (Freshly verified)
- **Protocol:** DS-TWR 1:3
- **Duration:** 65 seconds

## 2. Environment & Provisioning
All 4 boards were re-provisioned with the latest `main` branch firmware and DS configurations.

| Role | ST-Link SN | Config File | Status |
| :--- | :--- | :--- | :--- |
| TAG | `004500403234511137333934` | `tag_a1001_ds_3a_p2001.bin` | SUCCESS |
| ANC1 | `0031001F4142501820353451` | `anc_a2001_ds_p2001.bin` | SUCCESS |
| ANC2 | `0032002E3234510A37333934` | `anc_a2002_ds_p2001.bin` | SUCCESS |
| ANC3 | `001C00283234511137333934` | `anc_a2003_ds_p2001.bin` | SUCCESS |

## 3. UART Log Path
**Log Directory:** `artifacts/ioc_shell_post_merge_ds_smoke/20260429-1001/`

## 4. Verification Results
### TAG Performance (65s)
- **Total [TAG][MULTI]:** 391
- **ok=3/3:** 388 (**99.2% success rate**)
- **ok=2/3:** 1
- **ok=1/3:** 1
- **ok=0/3:** 1
- **Total [TAG][CYCLE]:** 391
- **overrun=0:** 391 (**100% timing stability**)

### ANCHOR Integrity
- **Role Dispatch:** All 3 anchors correctly identified as ANCHOR.
- **Protocol Mode:** All anchors entered DS mode as per configuration.
- **Errors:** Zero `bad_pan`, `bad_dst`, or system faults reported in anchor logs.

## 5. Final Judgment: PASS
The `main` branch is functionally stable following the merge. The post-merge smoke test confirms that the common firmware and cfg-based role dispatch mechanism are operating as intended on the production branch.

## 6. Observed Anomalies
- **None.** Minor packet loss (3 events) is within normal parameters.
- **Device ID:** ANC1/3 continue to report reference-level anomalous IDs (`0x20000000`/`0x00000000`), which remain non-blocking.

## 7. Next Action
- **Phase 14:** Final cleanup and documentation sync.
- **Merge ioc-shell-bulk-runtime-port branch deletion:** (Optional) Cleanup of feature branch.
