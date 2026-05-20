# 2026-05-20 UWB Config Apply Helper Customer Promotion Change Summary

- **Date**: 2026-05-20
- **Phase**: Phase 78F-CUSTOMER-UWB-CONFIG-HELPER-PROMOTION
- **Repository**: 03 Customer (`03_stm32-dw3000-G474RE-customer`)

---

## 1. Goal Description & Context

02 Product 저장소의 Phase 78D/E에서 정적/동적 검증이 완료된 `apply_uwb_cfg()` 함수 리팩토링 분할 구조를 03 Customer 저장소에 이식 승격시켰습니다. 이 과정에서 Customer 고유의 UART, SPI1_CS_Pin, RESET_Pin 및 저전력 기능 미사용(PRODUCT_LOW_POWER_MODE=0) 등의 정책을 그대로 보전하였습니다.

---

## 2. Changes Made

### 03 Customer Firmware Source Code

#### [MODIFY] [main.c](file:///c:/Users/ryan/dev/embedded/UWB_STM32/03_stm32-dw3000-G474RE-customer/Core/Src/main.c)
- **헬퍼 함수 프로토타입 선언** (Line 479-483 부근):
  ```c
  static void apply_uwb_radio_config(void);
  static void apply_uwb_identity_config(uwb_cfg_t *cfg);
  static void apply_uwb_network_config(void);
  static void apply_uwb_antenna_delay_config(void);
  static void apply_uwb_clock_or_diag_config(void);
  ```
- **`apply_uwb_cfg()` 모놀리식 구조 분할 및 오케스트레이션 적용** (Line 3192 부근):
  - `apply_uwb_radio_config()`, `apply_uwb_identity_config(cfg)`, `apply_uwb_network_config()`, `apply_uwb_antenna_delay_config()`, `apply_uwb_clock_or_diag_config()` 함수들로 개별 추출하여 구현 완료.

### Documentation & Task Checklist
- **`docs/task.md` 생성**: 03 Customer 프로젝트 고유의 Task Dashboard를 구축하고 Phase 78F 검증 성공 상태를 기록함.

---

## 3. Preserved Runtime Checklist

Customer 저장소에 리팩토링을 반영하며 아래의 고유 기기 정책이 침해 없이 완전히 유지됨을 확인하였습니다.

* **SPI/RESET 핀 매핑 (CS=UWB_SPI1_CS_Pin, RESET=UWB_RESET_Pin, hspi1) 유지**: 수정 없음 (PASS).
* **UART 통신 경로 (hlpuart1, printf route) 유지**: 디버그 UART 설정 및 로그 라우팅 변경 사항 없음 (PASS).
* **저전력 모드 차단 상태 (PRODUCT_LOW_POWER_MODE=0) 유지**: 저전력 모드 진입 시도 원천 차단 상태 유지 (PASS).
* **`[STEP3] apply_uwb_cfg done` 로그 위치 유지**: UWB 설정 최종 마무리 시점 정상 출력 보장 (PASS).
* **ID 검증 Failsafe**: Device ID 3회 재시도 및 무효 ID 거부 정책 변경 없음 (PASS).

---

## 4. Verification Results

### A. 빌드 검증
- **명령**: `cmake --build build/Release --clean-first`
- **결과**: **빌드 성공 (Warning 0)**
  ```text
  Memory region         Used Size  Region Size  %age Used
               RAM:        3568 B       128 KB      2.72%
             FLASH:       35280 B       512 KB      6.73%
  ```
  - 신규 헬퍼 기입 이후 컴파일 및 링킹 과정에서 warning이 단 1개도 나타나지 않았습니다.

### B. 딜리버리 신선도 검사
- **명령**: `powershell -File tools/check_delivery.ps1`
- **결과**: **성공 (PASS)**
  - 빌드 결과 생성된 `.bin`/`.hex` 파일이 `delivery/` 디렉터리에 신선하게 매칭 완료되었습니다.
