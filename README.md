# Current Status Snapshot (Pre-IOC Migration)

## 1. Stable Baseline
The current root project is considered a **frozen operational baseline**.

Validated:
- Workstream A (Operations Standardization) handoff completed
- Fixed firmware + replaceable `cfg.bin` operational model established
- Full provisioning / Config-only / Firmware-only flows validated
- 1:1 healthy ranging validated
- Near-contact ranging restored (`raw/q/clean_med/iir = 0`)
- Operator acceptance completed
- Package freeze completed

Deferred:
- Simultaneous 3-anchor validation
- Workstream B (link-layer / multi-anchor environmental debug)

## 2. Stable Project Rules
The current stable root project must be preserved as-is.

Do not reopen or modify without explicit reason:
- restored known-good UWB init path
- raw → quality gate → clean_med → iir processing order
- SS-TWR seq handling
- persistence / runtime role switching core behavior
- binary-only delivery model
- fixed firmware + cfg.bin replacement philosophy

## 3. IOC Migration Branch / Baseline Split
Baseline has been separated before IOC migration.

Git state:
- branch: `ioc-migration`
- tag: `pre-ioc-migration-baseline`

Meaning:
- root project = frozen stable baseline
- IOC migration work continues separately from this point

## 4. New IOC Shell Project
A new isolated IOC-based shell project has been created successfully under:

- `ioc_rebase/stm32-dw3000-g474re-ioc-shell/`

Seed IOC:
- `ioc_rebase/ioc_seed/stm_dw3000_seed.ioc`

Working IOC:
- `ioc_rebase/stm32-dw3000-g474re-ioc-shell/stm32-dw3000-g474re-ioc-shell.ioc`

Important:
- hardware-team seed pinout was preserved
- no UWB application code has been ported yet
- this shell was created only to establish a clean generated/buildable base

## 5. IOC Shell Build Status
The new IOC shell now configures and builds successfully.

Build result:
- `cmake -S . -B build -G Ninja ...` → success
- `cmake --build build` → success

Observed working condition:
- CMake needed explicit ARM toolchain/compiler configuration
- `CMAKE_SYSTEM_NAME=Generic`
- `CMAKE_SYSTEM_PROCESSOR=arm`
- explicit `arm-none-eabi-gcc/g++`
- generated CMake also required Cortex-M4F compile/link flags to avoid ARM-mode assembly errors

## 6. Important IOC Migration Notes
Do not assume the current root-project runtime pin usage is already aligned with the seed IOC.

Hardware-team seed IOC truth:
- `PA4` = `UWB_SPI1_CS`
- `PA15` = `UWB_RESET`
- `PA8` = `LED_RIGHT`

This differs from the current stable root `main.c` assumptions that were used during previous firmware work.
That mismatch must be handled intentionally during migration, not by blindly rewriting the seed IOC.

## 7. Next Task (Start of Real Migration)
The next phase is **code migration into the new IOC shell**, not more IOC generation.

Recommended migration order:
1. Compare generated shell structure vs stable root project
2. Port non-UWB-safe / low-risk generated-compatible files first
3. Port UWB support modules next
4. Integrate `main.c` last
5. Resolve pin mapping / BSP assumptions explicitly during integration
6. Rebuild after each migration step