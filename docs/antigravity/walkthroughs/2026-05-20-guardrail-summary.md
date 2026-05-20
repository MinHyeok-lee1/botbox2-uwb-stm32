# 2026-05-20 Customer UWB Config Apply Helpers Ordering Guardrail Change Summary

- **Date**: 2026-05-20
- **Phase**: Phase 79C-CUSTOMER-UWB-INIT-CONTRACT-GUARDRAIL-PROMOTION
- **Repository**: 03 Customer (`03_stm32-dw3000-G474RE-customer`)

---

## 1. Goal Description & Context

02 Product 저장소에서 검증 완료된 `apply_uwb_cfg()` 함수 및 그 헬퍼 함수군의 실행 순서 규약(ordering contract) 주석 가드레일을 03 Customer 배포 저장소에 승격 반영하였습니다. 

---

## 2. Customer Specific Policy & Protected Sections

* **Customer Delivery Policy**:
  - 본 저장소는 고객사 인도 목적의 최종 딜리버리 저장소(customer-facing delivery repo)이므로, 규명되지 않은 내부 하드웨어 실험이나 헬퍼 순서 변경은 엄격하게 통제(forbidden)되어야 함을 명시하였습니다.
* **Prototype Declarations (`Core/Src/main.c`)**:
  - `apply_uwb_cfg` 및 하위 5개 정적 헬퍼들 선언부에 5단계 순차 실행 계약에 대한 블록 주석 삽입.
* **Implementation (`Core/Src/main.c`)**:
  - `apply_uwb_cfg()` 함수 몸체에 순서 변경 및 병합 금지 경고 주석 배치.
  - 각 단계별 호출에 `/* [DO NOT REORDER] Step X: ... */` 마커 표시.
  - 저전력 standby 모드 미사용(`PRODUCT_LOW_POWER_MODE=0`) 상황에서도 UWB 거리 측정 및 내부 레지스터 시퀀스 동작을 위해 동일한 실행 순서가 강제됨을 주석화.

---

## 3. Build & Technical Verification

* **Compiler Results**: Clean compile with zero errors/warnings.
  - RAM: 3568 B / 128 KB (2.72%)
  - FLASH: 35280 B / 512 KB (6.73%)
* **Delivery Freshness**: `tools/check_delivery.ps1` -> `[OK] delivery/firmware.bin is fresh` (PASS).
* **Runtime Behavior**: 코드 바이너리 영향도가 없는 주석 변경이므로 기존 런타임 동작 유지 (Regression-Free).
