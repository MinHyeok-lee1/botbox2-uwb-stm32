# PRODUCT_PORTING_SNAPSHOT.md

## 관련 프로젝트 (Related Projects)
- **업스트림 (Upstream)**: 참조용 UWB 개발 프로젝트
- **다운스트림 (Downstream)**: 실제 제품 UWB 프로젝트

## 현재 제품 상태 (Current Product Status)
- **DS-TWR**: 제품 UWB 프로젝트에서 동작 확인 완료.
- **SS-TWR**: 제품 UWB 프로젝트에서 동작 확인 완료.
- **cfg 기반 프로비저닝 (cfg-based provisioning)**: 동작 확인 완료.
- **CAN 거리 데이터 송신 (CAN distance TX)**: 제품 UWB 보드에서 메인보드로의 송신 확인 완료.
- **FDCAN / Clock 설정**: 제품 전용 설정으로 조정 및 확인 완료.

## EVB에서 이식된 기능 (Imported from EVB)
- `cfg` 기반 `TAG/ANCHOR` 동작 모드
- 역할 분담 로직 (role dispatch)
- `SS-TWR` 경로 (path)
- `DS-TWR` 경로 (path)
- CAN 거리 데이터 소유권 정책 (CAN distance ownership policy)
- 클린 런타임 로그 정책 (clean runtime log policy)

## 제품 전용 차이점 (Product-specific Differences)
- `SystemClock_Config`: 제품 하드웨어 특성에 맞춰 의도적으로 다르게 설정됨.
- `FDCAN clock`: 제품 하드웨어 특성에 맞춰 의도적으로 다르게 설정됨.
- `FDCAN instance/routing`: 제품 하드웨어 배선에 맞춰 의도적으로 다르게 설정됨.
- 메인보드 CAN 통합: 제품 전용 사양으로 구현됨.
- `CubeMX/HAL` 생성 파일: 제품 전용 `.ioc` 설정에 기반함.

## 제품에서 제외된 항목 (Excluded from Product)
- 단방향 벤치마크 경로 (one-way benchmark path)
- 고주파수(high-Hz) 연구 트랙
- 기각된 미세 튜닝 후보군 (rejected micro-tuning candidates)
- 상세 진단 로그 (verbose diagnostic logs)
- EVB 전용 실험 기록 (EVB-only experiment records)

## 검증 증거 (Evidence Section)
- **빌드 (Build)**: 확인 완료 (confirmed)
- **프로비저닝 (Provisioning)**: 확인 완료 (confirmed)
- **SS-TWR**: 확인 완료 (confirmed)
- **DS-TWR**: 확인 완료 (confirmed)
- **CAN TX to mainboard**: 확인 완료 (confirmed)
- *상세 원천 UART/CAN Analyzer 로그는 추후 `artifacts/` 폴더에 첨부될 수 있음.*

## 검증 증거 아티팩트 (Evidence Artifacts)
- **증거 요약**: `artifacts/2026-04-29-product-porting-evidence-summary.txt`
- **CAN 패킷 테스트 벡터**: `artifacts/2026-04-29-product-porting-can-packet-vector.txt`
- *참고: 실제 UART/CAN Analyzer 로그가 확보되는 대로 위 아티팩트 리스트에 추가될 예정임.*

## 중요 경고 (Important Warning)
- 이 스냅샷은 새로운 성능 기준(performance baseline)이 아닙니다.
- 본 문서를 과거 EVB 기준 검증을 재오픈하는 용도로 사용하지 마십시오.
- 본 문서는 현재의 제품 이식 상태를 기록한 스냅샷으로 취급해야 합니다.
