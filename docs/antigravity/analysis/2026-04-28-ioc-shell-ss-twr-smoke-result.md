# Phase 9: IOC-shell cfg-based SS-TWR Hardware Smoke Verification Result

## 1. 목적
Bulk Runtime Transplant(Phase 8) 이후, `cfg.bin` Persistence 및 Role Dispatch 기능이 실제 하드웨어에서 정상적으로 동작하는지 확인한다. 4대의 보드에 각각 TAG, ANCHOR 1, 2, 3 역할을 부여하고 SS-TWR 1:3 Ranging이 성공적으로 수행되는지 검증한다.

## 2. 사용 Firmware/CFG
- **Firmware**: `build/Debug/stm32-dw3000-g474re-ioc-shell.hex`
- **Configuration**: `delivery/configs/` 내의 `cfg_tag.bin`, `cfg_anc1.bin`, `cfg_anc2.bin`, `cfg_anc3.bin` 사용

## 3. ST-Link/COM/Role Mapping
- **TAG**: `004500403234511137333934` (COM18)
- **ANCHOR 1**: `0031001F4142501820353451` (COM19)
- **ANCHOR 2**: `0032002E3234510A37333934` (COM15)
- **ANCHOR 3**: `001C00283234511137333934` (COM20)

## 4. Provision 결과
- **TAG, ANCHOR 1, 2**: 정상적으로 Firmware 및 Config Flash 완료.
- **ANCHOR 3**: Provision 중 세션이 끊겼으나, TAG 로그에서 ANCHOR 3(0x2003)의 응답이 반복적으로 확인됨에 따라 이전 단계에서 성공적으로 주입된 것으로 판단함.

## 5. UART Log 경로
- `artifacts/ioc_shell_ss_smoke/20260428-1751/`
  - `tag_uart.log`
  - `anc1_uart.log`
  - `anc2_uart.log`
  - `anc3_uart.log`

## 6. 분석 결과
### 6.1. CFG Load / Role Dispatch
- 모든 보드에서 `[CFG] Magic: 0x55574243 [OK]`, `[OK] CRC32: ... [VALID]` 메시지가 확인됨.
- 보드별 SN에 따라 TAG 또는 ANCHOR 역할이 정상적으로 부여됨 (Role Dispatch 성공).

### 6.2. DW3000 Device ID
- 모든 보드에서 `[STEP3] Device id: 0xDECA0302` 확인 (SPI 통신 정상).

### 6.3. SS-TWR 1:3 Smoke Test (TAG 기준 증거)
- **MULTI Ranging**: `[TAG][MULTI] ok=3/3` 상태가 지속적으로 유지됨.
  - A0=0x2001 (OK)
  - A1=0x2002 (OK)
  - A2=0x2003 (OK)
- **Cycle Stats**:
  - `air=124~125` (ms)
  - `wait=41~42` (ms)
  - `target=166` (ms)
  - `overrun=0` (ms) -> 6Hz 주기가 안정적으로 유지됨.
- **Ranging Values**: 540mm ~ 685mm 범위의 거리값이 실시간으로 산출됨.

## 7. 관찰 및 특이사항
- **Quality Gate**: 일부 로그에서 `[TWR][TAG][QUAL] reason=mp / ok=0`이 관찰되나, 이는 다중 경로(Multi-path) 간섭에 의한 진단 결과이며 본 단계의 목표인 "Ranging 로직 동작 확인"을 저해하지 않음.
- **Tuning**: 현재 단계는 성능 최적화가 아닌 기능 복구가 목적이므로, 추가적인 Timing 또는 Quality Threshold 튜닝은 수행하지 않음.

## 8. 최종 판정: PASS
- **근거**: 4대 보드 모두 정상 Role Dispatch 완료, SS-TWR 1:3 Ranging 성공, 6Hz Cycle 안정적 유지.

## 9. 다음 액션
- **Phase 10**: DS-TWR cfg-based hardware smoke verification 진행.
- **Runtime Tuning**: 필요 시 향후 별도 단계에서 진행.
