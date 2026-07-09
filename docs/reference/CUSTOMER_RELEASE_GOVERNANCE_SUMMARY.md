# Customer Release Governance Summary

## Runtime Acceptance Summary (2026-05-22)

This customer release is exported from the finalized Product runtime acceptance state.

- Product source commit: `5465025 fix(product): restore slow-listen entry visibility`
- Customer export commit: `3cda0a4 chore(customer): export slow-listen entry visibility`
- Slow-listen accepted window: `ANCHOR_LP_WAKE_INTERVAL_MS=2500U`, `ANCHOR_LP_LISTEN_WINDOW_MS=250U`
- Slow-listen entry logs:
  - `[ANC][PWR] enter low-power listen: no matched poll for 60000 ms`
  - `[ANC][RECOV] enter LOW_POWER_LISTEN: tick=... listen_win=250 sleep_win=2250`
- LED role acceptance: TAG OFF, `0x2001` RED, `0x2002` BLUE, `0x2003` YELLOW
- Distance output LPF: legacy `lowpassfilter.h` aligned; CAN/log path uses the filtered distance value
- Customer package scope is limited to customer-facing firmware, configuration, integrity metadata, and operating documentation.

Firmware artifact hashes:

| Artifact | Description |
| --- | --- |
| `delivery/firmware_product_ch5.bin` | CH5 제품용 기본 펌웨어 |
| `delivery/firmware_product_ch9.bin` | CH9 제품용 기본 펌웨어 |
| `delivery/firmware_rxdiag_ch5.bin` | CH5 진단용 펌웨어 |
| `delivery/firmware_rxdiag_ch9.bin` | CH9 진단용 펌웨어 |

## 1. 목적 (Purpose)
본 문서는 고객이 인도받은 릴리스 산출물을 확인하고, 해당 산출물의 무결성과 릴리스 가능 여부를 판단하는 기준을 이해하도록 돕는 요약 지침입니다. 제품의 기술적 무결성을 보장하기 위한 공식 승인 체계와 운영 절차를 설명합니다.

## 2. 적용 범위 (Scope)
- 고객 저장소에 배포된 최종 릴리스 산출물 패키지 (바이너리, 설정 파일, 매니페스트).
- 릴리스 무결성 확인 및 수용 판단 프로세스.

## 3. 고객 인도 산출물 확인 개요 (Delivery Verification)
고객은 인도받은 패키지 내 `delivery/` 폴더에 포함된 다음 항목들을 확인해야 합니다.
- **바이너리**: 펌웨어 실행 파일 (예: `firmware_product_ch5.bin`, `firmware_product_ch9.bin` 등 운영 프로필별 4종 변형 바이너리).
- **설정 파일**: 각 노드별 설정 바이너리 (`configs/*.bin`).
- **매니페스트**: 산출물 목록 및 해시 정보 (`manifest.txt`).
- **운영 가이드**: 런타임 설정 및 프로비저닝 안내 문서.

## 4. 산출물 무결성 확인 (Artifact Integrity Check)
산출물 무결성 확인은 인도받은 실제 파일들이 공식 매니페스트에 기록된 해시 값과 완벽히 일치하는지 검증하는 절차입니다.
- **확인 방법**: 고객 저장소 내 파이프라인 도구(`run_pipeline.ps1`)의 무결성 확인 모드를 실행합니다.
- **해석**: 모든 파일의 해시가 일치하면 무결성이 확보된 것으로 간주합니다. 해시 불일치 발생 시 해당 릴리스는 신뢰할 수 없으며 배포를 보류해야 합니다.

## 5. 릴리스 가능/보류 판단의 의미 (Release Readiness Verdict)
릴리스 검증 결과에 따라 다음과 같이 판단합니다.
- **릴리스 가능 (readiness=GO)**: 모든 기술적 검증과 정책 요건을 충족하여 제품 릴리스 승인이 완료된 상태입니다.
- **릴리스 보류 (readiness=NO-GO)**: 추가 확인이 필요하거나 기술적 이슈가 발견되어 배포가 승인되지 않은 상태입니다. 이 상태의 산출물은 운영 환경에 적용하지 않아야 합니다.

## 6. 고객 운영 수칙 (Operator Guidelines)
- **체크리스트 확인**: 모든 릴리스 산출물에 대해 무결성 확인이 완료되었는지 확인하십시오.
- **독립적 수용 판단**: 제품 릴리스 승인 결과를 바탕으로 고객 측면의 최종 수용 여부를 결정하십시오.
- **내부 절차 미수정**: 매니페스트 파일을 임의로 수정하거나 검증되지 않은 바이너리를 혼용하지 마십시오.

## 7. 지원 및 문의 (Support)
릴리스 산출물 확인 과정에서 해시 불일치나 기술적 의문 사항이 발생할 경우, 공식 기술 지원 채널을 통해 문의해 주시기 바랍니다.

## 8. 용어 정리 (Terminology)
- **릴리스 검증 결과**: 산출물의 기술적 무결성과 정책 정합성을 검증한 공식 증빙.
- **제품 릴리스 승인**: 릴리스 검증 결과를 바탕으로 배포 가능함을 공식적으로 확정하는 행위.
- **산출물 무결성 확인**: 매니페스트와 실제 산출물의 일치 여부를 검증하는 프로세스.

---
**최종 업데이트**: 2026-05-15
