# IOC-shell Migration Success Checkpoint

## 1. Overview
This document summarizes the successful migration of the UWB runtime and protocol logic from the reference repository (`stm32-dw3000-G474RE`) to the IOC-shell target repository (`stm32-dw3000-g474re-ioc-shell`). The migration has been validated through iterative phases including bulk transplant, build verification, and 1:3 hardware smoke testing for both SS-TWR and DS-TWR protocols.

- **Date:** 2026-04-28
- **Current Branch:** `ioc-shell-bulk-runtime-port`
- **Status:** **FUNCTIONALLY VALIDATED**

## 2. Completed Phases
The following milestones have been achieved in the current sprint:

### Phase 8: Bulk Runtime Transplant
- **Goal:** Port validated UWB logic while preserving the target's CubeMX-generated shell.
- **Outcome:** SUCCESS.
- **Key Actions:**
  - Integrated `main.c` UWB runtime logic into the target `main()`.
  - Ported `uwb_cfg`, `uwb_filter`, and `uwb_quality` modules.
  - Adapted pin mappings to target IOC symbols (PA4 for CS, PA15 for RESET, hlpuart1 for logging).
  - Restored `cfg.bin` persistence model at `0x0807F800`.

### Phase 9: SS-TWR 1:3 Hardware Smoke
- **Goal:** Verify SS-TWR ranging using the common firmware + specific configuration delivery model.
- **Outcome:** **PASS**.
- **Key Evidence:**
  - Successful `cfg.bin` load and role dispatch on 4 boards.
  - TAG log confirmed `ok=3/3` response from Anchors 0x2001, 0x2002, and 0x2003.
  - Stable 6Hz cycle maintained.

### Phase 10: DS-TWR 1:3 Hardware Smoke
- **Goal:** Verify DS-TWR ranging protocol support using the same golden firmware.
- **Outcome:** **PASS**.
- **Key Evidence:**
  - All boards correctly entered `Mode (SS/DS): 1`.
  - TAG log confirmed stable 1:3 ranging with `ok=3/3`.
  - `[TAG][CYCLE] overrun=0` confirmed timing stability in DS mode.

## 3. Core Success Indicators
| Indicator | Status | Verification Source |
| :--- | :--- | :--- |
| **Config Persistence** | SUCCESS | `[CFG] Flash config loaded successfully` in all logs |
| **Role Dispatch** | SUCCESS | Correct role assignment (TAG/ANCHOR) without recompilation |
| **Protocol Support** | SUCCESS | Both SS-TWR and DS-TWR functionally operational |
| **1:3 Multi-Anchor** | SUCCESS | Consistently polling and receiving 3 anchor responses |
| **Timing Stability** | SUCCESS | No cycle overruns at 6Hz baseline |

## 4. Preservation Conditions (Guardrails)
The following target-specific assets were strictly maintained during the migration:
- **Hardware Shell:** The original `.ioc` file and all CubeMX-generated initialization code (GPIO, SPI, UART, Clock) remain the authoritative source.
- **Pinmap Consistency:** No manual pin overrides; used target IOC symbols throughout.
- **Delivery Model:** Maintained the single golden firmware (`firmware.bin`) + board-specific configuration (`cfg.bin`) model.
- **No Premature Tuning:** Preserved reference timing constants; deferred higher-Hz or quality gate tuning to future phases.

## 5. Current Conclusion
The `ioc-shell` repository is now functionally equivalent to the reference repository regarding UWB ranging capabilities. The "Bulk Runtime Transplant" approach has proven successful in bridging the gap between the clean CubeMX project structure and the legacy UWB application logic.

## 6. Remaining Items
- **Phase 11: Long-term Stability Verification:** Perform extended burn-in tests to ensure no periodic drift or task collisions.
- **Host-side Validation:** Connect the UART output to host-side diagnostic tools for further data analysis.
- **Mainline Integration:** Devise a merge strategy to bring the validated `ioc-shell-bulk-runtime-port` changes back to the `main` branch.

---
**Checkpoint Document Concluded.**
