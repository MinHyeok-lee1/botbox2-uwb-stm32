# Hardware Bring-up Verification Plan (IOC-Shell Baseline)

This document provides a checklist to verify the successfully migrated `main.c` baseline on the `stm32-dw3000-g474re-ioc-shell` hardware.

## A. Pre-flash Verification

- [ ] **Successful Build**: Confirm that the build process completes without errors.
- [ ] **Artifact Path**: Identify the firmware file:
  - `delivery/firmware.bin`
  - `delivery/firmware.hex`
- [ ] **Role Expectation**: Check the build `ROLE` (TX or RX).
  - Note: `ROLE=TX` defaults to **TAG**, `ROLE=RX` defaults to **ANCHOR**.

## B. Flash and Serial Setup

- [ ] **Tool**: Use ST-Link (STM32CubeProgrammer or VS Code integration).
- [ ] **Target**: `delivery/firmware.hex`.
- [ ] **Serial Console**: Open the Virtual COM Port (Nucleo LPUART1).
- [ ] **Baud Rate**: `209700` (as defined in `MX_LPUART1_UART_Init`).
  - > [!WARNING]
  - > The current baud rate `209700` is non-standard. If garbage text appears, try `115200` or check for clock configuration mismatches.
- [ ] **Reset**: Press the Black (Reset) button on the Nucleo board after flashing.

## C. Expected Boot Log Sequence

The following logs should appear in order:

1. `[CFG] Flash load failed...` OR `[CFG] Flash config loaded successfully.`
2. `[STEP3] Device id: 0xDECA0302` (Crucial: Confirms SPI is working).
3. `[UWB][ANTD] tx_ant_dly=...`
4. `[UWB][CFG] chan=5 ...`
5. `[UWB][CLK] clk_off=...`
6. `[STEP3] apply_uwb_cfg done`
7. **Entry to Role Logic**:
   - TAG: `[TAG][MULTI] ok=x/y ...`
   - ANCHOR: `[RX][APP] Starting RX application`

## D. TAG-Mode Verification Steps

- [ ] Look for `[TAG][MULTI]` logs appearing periodically.
- [ ] If `ok=0/n`, the TAG is polling but finding no anchors.
- [ ] Look for `[TWR][TAG][QUAL]` logs if any anchor responds.
- [ ] **Failure Pattern**: `[TWR][TAG][TXFAIL]` or `[TWR][TAG][TXFRS_TO]` indicates hardware/timing issues.

## E. ANCHOR-Mode Verification Steps

- [ ] Look for `[A] RESP TX done` or `[RX FAIL]` logs.
- [ ] Look for `[ANC][STAT]` logs printed every 1 second.
- [ ] **Failure Pattern**: Constant `rx_err` or `rx_to` increases without any `rx_ok` indicates signal or configuration mismatch.

## F. First-level Blockers

If bring-up fails, check these first:

- **Serial Silence**:
  - Verify LPUART1 vs UART5.
  - Verify Baud Rate mismatch.
- **SPI Init Failure (`Unknown device id`)**:
  - Check `PA4` (CS) and `PA15` (RESET) physical connections.
  - Check SPI1 SCK (PA5), MISO (PA6), MOSI (PA7).
- **Communication Timeout**:
  - Ensure the transceiver is powered and the reset line is High (active-low reset).
  - Verify SPI speed (currently `42.5 MBits/s`, might need reduction for first-level stability).
- **Role Mismatch**:
  - If a TAG is flashed where an ANCHOR is expected, they will not range.
