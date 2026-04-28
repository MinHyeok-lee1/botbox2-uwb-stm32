# stm32-dw3000-g474re-ioc-shell Task Dashboard

## Project: main.c Migration Review and Patch
Current Status: `Patched / Ready for bring-up`

### Phase 1: Analysis & Review (Completed)
- [x] Analyze `main.c` for hardcoded pin mappings.
- [x] Compare old and new IOC files for pin changes.
- [x] Verify generated symbols in `main.h`.
- [x] Identify runtime dispatch logic gaps.

### Phase 2: Implementation Planning (Completed)
- [x] Create migration action report and implementation plan.
- [x] Obtain user approval for `main.c` patch.

### Phase 3: Execution (Completed)
- [x] Patch pin mapping macros in `main.c`.
- [x] Patch `main()` dispatch logic.
- [x] Verify localized changes in `main.c`.

### Phase 4: Verification (Completed)
- [x] Manual verification and walkthrough.
- [x] Patch `main.c` to enable `printf` over LPUART1 (COM Port).
- [x] Instrument `main.c` with early UWB bring-up debug prints.

### Phase 5: Post-Patch Status Audit (Completed)
- [x] Conduct IOC-shell post-patch bring-up status audit to synchronize documentation with current code state.

### Phase 6: Clean Build Verification (Completed)
- [x] Execute clean build in the repository root using CMake presets.
- [x] Verify successful compilation and linking without modifying source files.
- [x] Document build verification results.

### Phase 7: IOC-shell Flash/UART + SS/DS TWR Smoke Verification (Failed)
- [x] Flash firmware to TAG (`004500403234511137333934`) and 3 ANCHORs.
- [x] Capture and analyze UART boot and runtime logs.
- [ ] Verify SS-TWR/DS-TWR smoke test.
- **Result: FAIL**. Primary cause: cfg.bin persistence 미연결로 4대 모두 TAG fallback. Smoke 조건 미성립.
- **Secondary (보류)**: ANC1/ANC3 Device ID anomaly — role dispatch 정상화 후 재확인.
- **다음 액션**: cfg.bin persistence / role dispatch parity 복구.

### Phase 8: Bulk Runtime Transplant from Reference Repo (Completed)
- [x] Create branch `ioc-shell-bulk-runtime-port`.
- [x] Import validated source runtime logic into target `main.c` while preserving target CubeMX init and clock configuration.
- [x] Import/update `uwb_cfg`, `uwb_filter`, `uwb_quality`, selected DW3000 driver files, delivery configs, cfg presets, and provisioning/check tooling.
- [x] Adapt DW3000 mapping to target IOC pin symbols: CS=`UWB_SPI1_CS` PA4, RESET=`UWB_RESET` PA15, SPI=`hspi1`, printf UART=`hlpuart1`.
- [x] Restore cfg persistence / role dispatch path using `cfg.bin` at `0x0807F800`.
- [x] Verify Debug build.
- **Result: PASS**. `cmake --preset Debug` and `cmake --build build/Debug` completed successfully.
- **Artifacts**: `build/Debug/stm32-dw3000-g474re-ioc-shell.elf`, `.bin`, `.hex`; delivery copies refreshed as `delivery/firmware.bin` and `delivery/firmware.hex`.
- **Next**: hardware flash not performed in this phase; run cfg-specific SS/DS UART smoke verification next.

### Phase 9: cfg-based SS-TWR Hardware Smoke Verification (Completed)
- [x] Provision 4 boards with unique `cfg.bin` using `tools/provision.ps1`.
- [x] Capture UART logs for TAG and 3 ANCHORs.
- [x] Verify cfg load and role dispatch (TAG, ANCHOR 1, 2, 3).
- [x] Verify DW3000 device ID on all boards.
- [x] Verify SS-TWR 1:3 smoke test results (ok=3/3, stable cycle).
- **Result: PASS**. All anchors responding to TAG; Ranging logic fully functional in IOC-shell.
- **Log Path**: `artifacts/ioc_shell_ss_smoke/20260428-1751/`
- **Next**: Phase 10: DS-TWR cfg-based smoke verification.

### Phase 10: cfg-based DS-TWR Hardware Smoke Verification (Completed)
- [x] Provision 4 boards with DS-specific `cfg.bin` using `tools/provision.ps1`.
- [x] Capture UART logs for TAG and 3 ANCHORs (60s+).
- [x] Verify cfg load and DS mode entry (`Mode (SS/DS): 1`).
- [x] Verify DS-TWR 1:3 smoke test results (`ok=3/3`, `overrun=0`).
- **Result: PASS**. Stable 1:3 DS-TWR ranging confirmed.
- **Log Path**: `artifacts/ioc_shell_ds_smoke/20260428-1822/`
- **Next**: Phase 11: Long-term stability verification.
