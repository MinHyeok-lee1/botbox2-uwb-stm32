# 2026-04-01 - main.c Migration Analysis

## Overview
This document analyzes the mismatches in `main.c` after migrating from `NUCLEO-G474-DWM3000EVB-Example.ioc` to `stm32-dw3000-g474re-ioc-shell.ioc`.

## Findings

### 1. Pin Mappings
The current `main.c` (at 127faf3 baseline) uses hardcoded pin ports and pins that match the old Nucleo layout but are incorrect for the new shell.

| Signal | Old (main.c) | New (IOC / main.h) | Status |
| :--- | :--- | :--- | :--- |
| **DW3000 CS** | `GPIOB`, `GPIO_PIN_6` | `GPIOA`, `GPIO_PIN_4` (`UWB_SPI1_CS`) | **Mismatch** |
| **DW3000 RESET** | `GPIOA`, `GPIO_PIN_8` | `GPIOA`, `GPIO_PIN_15` (`UWB_RESET`) | **Mismatch** |
| **DW3000 IRQ** | `PA9` (implicit) | `PC4` (`UWB_IRQ`) | **Mismatch** |
| **DW3000 WAKEUP** | `PC7` (implicit) | `PC10` (`UWB_WAKEUP`) | **Mismatch** |
| **DW3000 EXTON** | N/A | `PC11` (`UWB_EXTON`) | **New** |

### 2. Runtime Flow
The `main()` function in `main.c` initializes the hardware and loads the configuration but ends in an empty `while(1)` loop. It does not dispatch to the active role logic (`tag_main()` or `anchor_main()`).

### 3. Peripheral Initialization
- `MX_SPI1_Init()` is called correctly.
- `MX_LPUART1_UART_Init()` is called correctly.
- `MX_TIM2_Init()` is called correctly for `micros()`.

## Impact Assessment
- **UWB Communication**: Will fail because CS and RESET pins are incorrect, preventing SPI communication and device reset.
- **Ranging Flow**: Will not start because the main loop is empty.
- **Interrupts**: No immediate impact as `main.c` currently uses polling, but the physical connection is moved.
