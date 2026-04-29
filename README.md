# STM32 DW3000 IOC-Shell (Current Status: Integrated)

## 1. Overview
This repository contains the integrated UWB ranging firmware for the STM32G474RE + DW3000 platform, built upon a clean CubeMX-generated "IOC Shell". The UWB application logic has been successfully ported from the reference repository and validated for high-stability ranging.

## 2. Current Project Status
- **Baseline:** Integrated with `main` branch.
- **Hardware Abstraction:** Managed via `.ioc` (STM32CubeMX Authority).
- **Core Logic:** Multi-anchor UWB runtime (SS-TWR & DS-TWR support).
- **Provisioning:** Configuration-driven role dispatch (TAG/ANCHOR) via `cfg.bin` at `0x0807F800`.

## 3. Validated Capabilities
- **Multi-Anchor Ranging:** Stable 1:3 DS-TWR ranging at 6Hz baseline.
- **Protocol Stability:** >99.9% ranging success rate in burn-in tests.
- **Timing Integrity:** 100% cycle stability (zero task overruns).
- **Build System:** Clean CMake build using predefined presets.

## 4. Hardware Configuration (IOC Authority)
- **SPI1:** DW3000 Communication (`PA4`=CS, `PA15`=RESET).
- **LPUART1:** Debug logging (115200 8N1).
- **LEDs:** System status indicators (`PA8`=LED_RIGHT).

## 5. Deployment Model
The project follows a **Golden Firmware** model:
1. **firmware.bin:** A single common binary deployed to all boards.
2. **cfg.bin:** A small (40-byte) configuration file that determines the board's role (TAG/ANCHOR), ID, and ranging parameters at runtime.

## 6. Documentation
Detailed migration history and verification results are available in:
- `docs/task.md`: Active task dashboard.
- `docs/antigravity/analysis/`: Phase-specific verification reports (Smoke tests, Burn-in results, Integration checkpoints).

---
*Last Updated: 2026-04-29*