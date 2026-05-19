# 2026-05-19 Low-Power / Standby Wake Path Customer Propagation Summary

## 1. Overview
The low-power standby wake and product CAN runtime path, originally validated in the Product repository (`02`), has been successfully ported to the Customer repository (`03_stm32-dw3000-G474RE-customer`) as a minimal diff. No ranging algorithms, schedulers, or CAN protocol logics have been altered. The codebase has been fully verified to build cleanly under `Release` configuration.

---

## 2. Ported Feature Checklist
All required items specified in the implementation criteria have been integrated:
*   [x] **Hardware Pins Reconfiguration (gpio.c):** Split `USER_BUTTON_Pin` (PC13) from shared inputs to configure it as `GPIO_PULLUP`. Configured `MODE_AUTO_Pin` (PB4) and `MODE_MANUAL_Pin` (PB5) as `GPIO_PULLUP` (instead of `GPIO_NOPULL`). Other pins (e.g. `SW1`, `SW2`) remain completely unchanged (`GPIO_NOPULL`).
*   [x] **Active-Low Button Helper (main.c):** Added inline helper `product_gpio_pressed_low()` which correctly checks for `GPIO_PIN_RESET`.
*   [x] **Debounce and Grace Period (main.c):** Integrated a 1000ms grace period after boot/wake before any manual low-power requests can trigger. Implemented a robust 50ms debounce logic using the active-low helper base inside `product_low_power_request_pending()`.
*   [x] **Held-Button Release Guards (main.c):** Included release loops (`while (product_gpio_pressed_low(...))`) for:
    *   `PC13` wake-up button on standby wake boot and STOP1 wake recovery.
    *   `PB5` manual low-power request button prior to entering STOP1 or STANDBY modes.
*   [x] **STANDBY Wake Boot Cause Gate (main.c):** Modified `main()` entry check to inspect `PWR_FLAG_SB` and `PWR_FLAG_WUF2`.
*   [x] **Logs and Auto STANDBY Skip Logic (main.c):**
    *   Prints diagnostic wake logs matching the expected sequences.
    *   Clears wakeup flags after detection.
    *   If a standby wake is detected, prints `[BOOT] Auto STANDBY skipped after standby wake boot` and skips auto-standby entry.
    *   If cold power-on is detected and auto-entry is enabled, prints `[BOOT] Auto-entering STANDBY mode...` and puts the device to sleep immediately.
*   [x] **Build Options Propagation (CMakeLists.txt):** Updated `CMakeLists.txt` to pass both `-DPRODUCT_LOW_POWER_MODE` and `-DPRODUCT_LOW_POWER_AUTO_ENTRY` flags dynamically into compiler preprocessors.

---

## 3. Git Diff Statistics

```
 CMakeLists.txt        |    5 +
 Core/Src/gpio.c       |   12 +-
 Core/Src/main.c       |   73 +-
 delivery/firmware.bin |  Bin 34488 -> 34680 bytes
 delivery/firmware.hex | 3966 +++++++++++++++++++++++++------------------------
 5 files changed, 2071 insertions(+), 1985 deletions(-)
```

---

## 4. Compile & Build Verification
The firmware has been compiled under the exact requested configuration:
```powershell
# Configure CMake with Standby Mode (2) and Auto-Entry (1)
cmake --preset Release -DPRODUCT_LOW_POWER_MODE=2 -DPRODUCT_LOW_POWER_AUTO_ENTRY=1

# Compile with parallel jobs
cmake --build build/Release --parallel 8
```

### Build Result Output:
*   **Compilation:** Clean, no warnings or errors.
*   **Flash Size:** `34,680 Bytes` (approx. `6.61%` of the `512 KB` region).
*   **RAM Size:** `3,552 Bytes` (approx. `2.71%` of the `128 KB` region).
*   **Artifact Generation:** `.bin` and `.hex` binaries have been copied to `delivery/firmware.bin` and `delivery/firmware.hex` respectively.

---

## 5. Provisioning & Flashing Commands

### Flashing via STM32CubeProgrammer CLI
To flash the compiled release firmware to the STM32G474RE target:
```powershell
# Erase and write the new firmware
STM32_Programmer_CLI.exe -c port=SWD -w delivery/firmware.bin 0x08000000 -v -rst
```

### Provisioning Compiled Options
If you wish to modify compile-time behavior, configure the build dynamically:
*   **Standby Low Power with Auto-Standby enabled on cold boot (Default test):**
    ```powershell
    cmake --preset Release -DPRODUCT_LOW_POWER_MODE=2 -DPRODUCT_LOW_POWER_AUTO_ENTRY=1
    ```
*   **Stop Low Power with Auto-Stop disabled (Manual only):**
    ```powershell
    cmake --preset Release -DPRODUCT_LOW_POWER_MODE=1 -DPRODUCT_LOW_POWER_AUTO_ENTRY=0
    ```
*   **Low Power logic fully compiled out:**
    ```powershell
    cmake --preset Release -DPRODUCT_LOW_POWER_MODE=0
    ```

---

## 6. Proposed Commit Message
```
feat(power): promote standby wake runtime path to customer firmware
```
