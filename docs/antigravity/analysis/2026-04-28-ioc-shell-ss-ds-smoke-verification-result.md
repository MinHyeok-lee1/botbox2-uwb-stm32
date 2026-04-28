# Phase 7: IOC-shell Flash/UART + SS/DS TWR Smoke Verification Result

## 1. 목적
현재 이식된 `stm32-dw3000-g474re-ioc-shell` 프로젝트 펌웨어를 4대의 ST-Link 보드(TAG 1대, ANCHOR 3대)에 플래시하고, UART Boot Log를 통해 UWB 모듈 초기화(Device ID) 및 기본 SS/DS-TWR 동작 가능 여부를 스모크 테스트(Smoke Test) 수준에서 확인한다.

## 2. 사용한 Build Artifact
- `build/Debug/stm32-dw3000-g474re-ioc-shell.hex`

## 3. ST-Link SN / 역할 매핑
- **TAG**: `004500403234511137333934`
- **ANCHOR 1**: `0031001F4142501820353451`
- **ANCHOR 2**: `0032002E3234510A37333934`
- **ANCHOR 3**: `001C00283234511137333934`

## 4. COM 포트 매핑
- **TAG**: COM18
- **ANCHOR 1**: COM19
- **ANCHOR 2**: COM15
- **ANCHOR 3**: COM20

## 5. Flash 결과
- **PASS**: STM32CubeProgrammer CLI를 통해 4대 모두 정상적으로 `stm32-dw3000-g474re-ioc-shell.hex` 플래시 성공 및 Software Reset 완료.

## 6. UART Log 파일 경로
- `artifacts/ioc_shell_bringup/20260428-1723/tag_uart.log`
- `artifacts/ioc_shell_bringup/20260428-1723/anc1_uart.log`
- `artifacts/ioc_shell_bringup/20260428-1723/anc2_uart.log`
- `artifacts/ioc_shell_bringup/20260428-1723/anc3_uart.log`

## 7. Boot / DW3000 Device ID 판정
- **TAG**: **PASS** (`[STEP3] Device id: 0xDECA0302`)
- **ANCHOR 1**: **REVIEW** (`[STEP3] Device id: 0x80000000`) — Secondary symptom. Role dispatch 정상화 후 재확인 필요.
- **ANCHOR 2**: **PASS** (`[STEP3] Device id: 0xDECA0302`)
- **ANCHOR 3**: **REVIEW** (`[STEP3] Device id: 0x00000000`) — Secondary symptom. Role dispatch 정상화 후 재확인 필요.

> **참고**: ANC1, ANC3의 Device ID anomaly는 기존 프로젝트에서 정상 동작하던 보드이므로 하드웨어/SPI 고장으로 단정하지 않는다. cfg persistence/role dispatch 정상화 후 재확인할 secondary symptom으로 분류한다.

## 8. Role Dispatch 상태
- **FAIL**: 4대의 보드 모두 플래시 메모리에서 환경 설정을 불러오지 못하고 기본 컴파일된 역할(TAG)로 Fallback 됨 (`[CFG] Flash load failed. Falling back to compiled role...`). 결과적으로 모든 보드가 `Role: TAG`로 동작을 시도함.

## 9. SS-TWR 1:1 Smoke 결과
- **FAIL**: 모든 보드가 TAG로 부팅되어 응답(RESP)을 줄 수 있는 ANCHOR가 없음. 계속해서 `[TAG][MULTI] ok=0/3 A0=0x2001:FAIL...` 로그만 반복 출력됨. 통신 불가능.

## 10. DS-TWR 1:1 Smoke 결과
- **FAIL**: 상동.

## 11. SS-TWR 1:3 Smoke 결과
- **FAIL**: 상동.

## 12. DS-TWR 1:3 Smoke 결과
- **FAIL**: 상동.

## 13. PASS / REVIEW / FAIL 최종 판정
- **최종 판정: FAIL**

## 14. 실패 또는 보류 사유

### 1차 Root Cause (Primary)
**cfg.bin Persistence / Role Dispatch 부재**: 동일한 `.hex`를 4대에 플래시했으며, 각 보드의 역할은 cfg.bin Flash persistence 또는 compile-time fallback에 의해 결정되어야 한다. 현재 IOC-shell 프로젝트에 cfg persistence가 아직 연결되지 않았거나 cfg load address/format/provisioning이 미완성이므로, 모든 보드에서 `[CFG] Flash load failed`가 발생하고 compiled fallback role(TAG)로 부팅되었다. ANCHOR 역할을 수행하는 보드가 없어 SS/DS-TWR smoke test 조건 자체가 성립하지 않았다.

### 2차 증상 (Secondary — 보류)
**ANC1, ANC3 Device ID anomaly**: ANCHOR 1(`0x80000000`)과 ANCHOR 3(`0x00000000`)에서 정상적인 DW3000 Device ID를 읽지 못했다. 단, 해당 보드들은 기존 프로젝트(source repo)에서 정상 동작이 확인된 하드웨어이므로 SPI 고장 또는 하드웨어 불량으로 단정하지 않는다. Role dispatch가 정상화된 후 재확인할 secondary symptom으로 분류한다.

> **주의**: TAG SN/COM 매핑(`004500403234511137333934` / COM18)은 정확하다. 실패 원인을 "TAG 보드에 잘못 flash함"으로 해석하면 안 된다.

## 15. 다음 액션
1. **cfg.bin Persistence / Role Dispatch Parity 복구**: IOC-shell 프로젝트에서 cfg load/save 경로가 올바르게 연결되어 있는지 점검하고, 각 보드에 개별 역할(TAG, ANCHOR 1/2/3)을 부여할 수 있는 프로비저닝 메커니즘을 확보한다.
2. **Role Dispatch 정상화 후 재테스트**: cfg persistence가 복구되면 Phase 7 smoke test를 재실행하여 SS/DS-TWR 동작을 확인한다.
3. **ANC1/ANC3 Device ID anomaly 재확인**: Role dispatch 정상화 후에도 Device ID 문제가 재현되면 그때 하드웨어/SPI 연결 상태를 점검한다.
