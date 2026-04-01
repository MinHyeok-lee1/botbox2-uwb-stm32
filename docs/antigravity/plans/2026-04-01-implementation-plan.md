# 2026-04-01 - main.c Migration Implementation Plan

This plan identifies and patches the mismatches in `Core/Src/main.c` following the migration to `stm32-dw3000-g474re-ioc-shell.ioc`.

# User Review Required

> [!IMPORTANT]
> The DW3000 pins CS (PA4) and RESET (PA15) have moved. Applying this patch is critical for SPI communication with the transceiver.

> [!WARNING]
> The `main()` loop currently reaches an empty `while(1)`. This patch will dispatch the execution to `tag_main()` or `anchor_main()` based on the runtime configuration.

## 1. Exact `main.c` mismatches caused by IOC migration

- **CS Definition (must-fix)**:
  - hardcoded `GPIOB` / `GPIO_PIN_6` (Line 398-399)
- **RESET Definition (must-fix)**:
  - hardcoded `GPIOA` / `GPIO_PIN_8` (Line 401-402)
- **Empty main loop (must-fix)**:
  - `main()` ends in an empty `while(1)` at Line 1655.
- **Peripheral init order (should-fix)**:
  - `MX_*_Init()` functions are mostly correct, but `dw3000_reset()` and `uwb_lib_init()` are called before the main loop, which is acceptable, but the dispatch must follow.

## 2. Pin mapping patch plan

Replace hardcoded DW3000 pin macros with generated `main.h` symbols:

```diff
- #define DW3000_CS_PORT     GPIOB
- #define DW3000_CS_PIN      GPIO_PIN_6
- #define DW3000_RST_PORT    GPIOA
- #define DW3000_RST_PIN     GPIO_PIN_8
+ #define DW3000_CS_PORT     UWB_SPI1_CS_GPIO_Port
+ #define DW3000_CS_PIN      UWB_SPI1_CS_Pin
+ #define DW3000_RST_PORT    UWB_RESET_GPIO_Port
+ #define DW3000_RST_PIN     UWB_RESET_Pin
```

*Confirmed generated symbol names in [main.h](file:///c:/Users/ryan/dev/embedded/stm32-dw3000-g474re-ioc-shell/Core/Inc/main.h).*

## 3. Runtime execution flow patch plan

Insert role dispatch logic in `main()` immediately after `apply_uwb_cfg()` and before the main background loop.

**Minimal insertion point:**
After `printf("[STEP3] apply_uwb_cfg done\r\n");` (Line 1649).

**Proposed change:**
```c
  if (runtime_role == UWB_ROLE_TAG) {
      tag_main();
  } else {
      anchor_main();
  }
```

## 4. IRQ / WAKEUP / EXTON impact review

- **UWB IRQ (PC4)**:
  - `main.c` currently uses polling (`while(1)` on `SYS_STATUS_ID`).
  - **Verdict**: No `main.c` change required now; defer until interrupt-based path is enabled.
- **UWB WAKEUP (PC10)**:
  - `main.c` does not currently use sleep-wake cycles.
  - **Verdict**: No `main.c` change required now.
- **UWB EXTON (PC11)**:
  - Not referenced in `main.c`.
  - **Verdict**: No `main.c` change required now.

## 5. Minimal patch proposal

### [MODIFY] [main.c](file:///c:/Users/ryan/dev/embedded/stm32-dw3000-g474re-ioc-shell/Core/Src/main.c)

#### Chunk 1: DW3000 Pin Mapping
```c
/* ===== DW3000 pin mapping ===== */
#define DW3000_CS_PORT     UWB_SPI1_CS_GPIO_Port
#define DW3000_CS_PIN      UWB_SPI1_CS_Pin

#define DW3000_RST_PORT    UWB_RESET_GPIO_Port
#define DW3000_RST_PIN     UWB_RESET_Pin
/* ========================================= */
```

#### Chunk 2: Runtime Role Dispatch
```c
  printf("[STEP3] apply_uwb_cfg done\r\n");

  /* Call runtime role logic (infinite loops) */
  if (runtime_role == UWB_ROLE_TAG) {
      tag_main();
  } else {
      anchor_main();
  }

  /* USER CODE END 2 */
```

## 6. Risk notes

- **`spi.c` Verification**: SPI1 settings should be checked. Current IOC shows 42.5 MBits/s which might be too fast for bring-up.
- **`stm32g4xx_it.c`**: Future IRQ handlers.
- **`gpio.c`**: `PA4` initialization state.

## Verification Plan

### Automated Tests
- Build project using CMake.

### Manual Verification
- Deploy to device and check serial terminal for device bring-up and logs.
