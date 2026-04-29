# IOC-shell Bulk Runtime Transplant Result

Date: 2026-04-28

## 1. Purpose

Port the validated UWB runtime, runtime cfg persistence, role dispatch, delivery configs, and operator tooling from the reference repo into the IOC shell target while preserving the target IOC/CubeMX generated hardware shell.

## 2. Imported From Source

- `Core/Src/main.c` UWB USER runtime logic, integrated into target generated init shell
- `Core/Src/uwb_cfg.c`
- `Core/Inc/uwb_cfg.h`
- `Core/Src/uwb_filter.c`
- `Core/Inc/uwb_filter.h`
- `Core/Src/uwb_quality.c`
- `Core/Inc/uwb_quality.h`
- `Drivers/decadriver/deca_device.c`
- `Drivers/decadriver/deca_device_api.h`
- `tools/provision.ps1`
- `tools/check_delivery.ps1`
- `delivery/tools/provision.ps1`
- `delivery/tools/check_delivery.ps1`
- `delivery/configs/*_ss_*.bin`
- `delivery/configs/*_ds_*.bin`
- `cfg/presets/*`
- `docs/RUNTIME_CONFIG_GUIDE.md`
- `docs/OPERATOR_FLASH_UART_RUNBOOK.md`
- `delivery/docs/RUNTIME_CONFIG_GUIDE.md`

## 3. Preserved Target Areas

- `stm32-dw3000-g474re-ioc-shell.ioc`
- target generated `Core/Inc/main.h` pin symbols
- target generated `Core/Src/stm32g4xx_hal_msp.c`
- target `SystemClock_Config`
- target `MX_GPIO_Init`, `MX_SPI1_Init`, `MX_LPUART1_UART_Init`, `MX_FDCAN1_Init`, `MX_UART5_Init`, `MX_FMAC_Init`, `MX_TIM2_Init`
- target startup, linker script, CMake presets, and CubeMX CMake shell

## 4. Pinmap Adaptation

- DW3000 CS uses target IOC symbol `UWB_SPI1_CS_GPIO_Port` / `UWB_SPI1_CS_Pin` on PA4.
- DW3000 RESET uses target IOC symbol `UWB_RESET_GPIO_Port` / `UWB_RESET_Pin` on PA15.
- DW3000 SPI uses target `hspi1`.
- printf/UART logging uses target `hlpuart1` through `__io_putchar`.
- `UWB_IRQ_Pin` / `UWB_IRQ_GPIO_Port` remain available from target IOC (`PC4`) but the transplanted runtime keeps the current polling-oriented path.

## 5. Config Persistence And Role Dispatch

- `uwb_cfg_load`, `uwb_cfg_save`, magic/version/length/CRC validation, and flash cfg address `0x0807F800` are included in the target build.
- Boot loads `cfg.bin` from flash and dispatches runtime role by `cfg.role`.
- Invalid/missing cfg falls back to compiled role only as recovery.
- TAG/ANCHOR, SS/DS mode, PAN ID, short address, anchor count, and anchor list remain cfg-driven.

## 6. CMake And Build Changes

- Existing target CMake structure was preserved.
- Existing optional UWB module inclusion in `cmake/stm32cubemx/CMakeLists.txt` is sufficient for `uwb_cfg/filter/quality`.
- Root and delivery `check_delivery.ps1` scripts were adapted for target artifact name `stm32-dw3000-g474re-ioc-shell` and Debug output.

## 7. Build Command And Result

Commands:

```powershell
cmake --preset Debug
cmake --build build/Debug
powershell -ExecutionPolicy Bypass -File .\tools\check_delivery.ps1
powershell -ExecutionPolicy Bypass -File .\delivery\tools\check_delivery.ps1
```

Result:

- Configure: success
- Build: success
- Generated artifacts:
  - `build/Debug/stm32-dw3000-g474re-ioc-shell.elf`
  - `build/Debug/stm32-dw3000-g474re-ioc-shell.bin`
  - `build/Debug/stm32-dw3000-g474re-ioc-shell.hex`
  - `delivery/firmware.bin`
  - `delivery/firmware.hex`
- Memory report: RAM 3472 B / 128 KB, FLASH 52944 B / 512 KB

## 8. Remaining Hardware Verification

- No hardware flash was performed in this phase.
- Next verification should flash common `delivery/firmware.bin` plus board-specific cfg bins.
- Confirm UART boot logs show cfg load success, correct role, PAN, short address, ranging mode, and anchor list.
- Run SS 1T3A smoke first, then DS 1T3A smoke using imported configs.

## 9. Notes

- The target IOC and generated pinmap remain the authority.
- Source runtime timing constants were preserved; no higher-Hz tuning was introduced.
- A small source comment/token repair was required where mojibake comments swallowed C tokens during transplant.
