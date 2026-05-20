# 2026-05-20 UWB Config Apply Helper Customer Promotion Plan

- **Date**: 2026-05-20
- **Phase**: Phase 78F-CUSTOMER-UWB-CONFIG-HELPER-PROMOTION
- **Repository**: 03 Customer (`03_stm32-dw3000-G474RE-customer`)

---

## Goal Description

02 Product 저장소에서 리팩토링 검증을 마친 `apply_uwb_cfg()` 내의 static 헬퍼 분리 구조를 03 Customer 저장소로 승격 전파합니다. 이 승격은 순수 소스 코드 가독성 향상 목적이며, Customer 고유의 기기 정책(저전력 사용하지 않음, UART hlpuart1 출력, hspi1/RESET/CS 핀 매핑 등)은 100% 보존합니다.

---

## Proposed Changes

### Core/Src/main.c
- **헬퍼 함수 프로토타입 추가**:
  ```c
  static void apply_uwb_radio_config(void);
  static void apply_uwb_identity_config(uwb_cfg_t *cfg);
  static void apply_uwb_network_config(void);
  static void apply_uwb_antenna_delay_config(void);
  static void apply_uwb_clock_or_diag_config(void);
  ```
- **함수 구조 정의**:
  - `apply_uwb_radio_config()`, `apply_uwb_identity_config()`, `apply_uwb_network_config()`, `apply_uwb_antenna_delay_config()`, `apply_uwb_clock_or_diag_config()`를 추가합니다.
  - `apply_uwb_cfg(cfg)` orchestrator가 이들을 순서대로 호출하도록 변경합니다.

---

## Verification Plan

### Automated Tests
- `cmake --build build/Release --clean-first` 실행을 통해 경고 및 컴파일 오류가 없는지 검증합니다.
- `powershell -File tools/check_delivery.ps1`을 통해 빌드 생성물이 delivery 폴더에 최신 상태로 배치되었는지 확인합니다.
