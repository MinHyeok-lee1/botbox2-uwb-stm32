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
