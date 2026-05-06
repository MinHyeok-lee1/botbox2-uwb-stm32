# 제품 개요 (PRODUCT OVERVIEW): STM32 DW3000 UWB 펌웨어

## 1. 개요 (Overview)
본 프로젝트는 STM32G474RE 마이크로컨트롤러와 DW3000 UWB 트랜시버를 사용하는 실제 제품용 UWB 펌웨어입니다. 
STM32CubeMX(IOC Shell)를 기반으로 구축되었으며, 실제 하드웨어 보드 통합 및 제품 수용(Product Acceptance)을 목적으로 관리됩니다.

## 2. 프로젝트 역할 및 관계 (Project Roles and Relationships)
- **제품 프로젝트 (본 저장소)**:
  - 실제 제품 보드 통합 및 핀맵 결정 소유.
  - 제품 수용 증거(Product Acceptance evidence) 및 검증 기록 보존.
  - 제품 프로비저닝 및 현장 운영 가이드 제공.
- **업스트림 참조 프로젝트 (Upstream Reference)**:
  - UWB 알고리즘 검증, 성능 벤치마크, 참조 구현 제공.
  - 심층 거버넌스 이력 및 내부 기술 원서 정보 관리.

본 프로젝트는 의도적으로 EVB 프로젝트와 동일하지 않게 유지됩니다. 보드 전용 설정(Clock, FDCAN, pinmux 등)은 제품 보드의 특성에 맞게 최적화되어 있습니다.

## 3. 주요 기능 (Key Capabilities)
- **설정 기반 역할 자동 할당**: `cfg.bin`을 통해 런타임에 TAG 또는 ANCHOR로 동작.
- **SS-TWR (Single-Sided Two-Way Ranging)** 지원.
- **DS-TWR (Double-Sided Two-Way Ranging)** 지원.
- **CAN 거리 정보 송신**: 측정된 거리 데이터를 메인보드로 실시간 전송.
- **제품별 통합 (Product-specific Integration)**: 보드 특성에 맞춘 FDCAN 및 시스템 클럭 최적화.

## 4. 문서 운영 정책 (Documentation Policy)
- **제품 지향 (Product-facing)**: 본 프로젝트의 문서는 운영자와 개발자가 이해하기 쉽도록 간결하게 유지됩니다.
- **참조 기반 (Reference-based)**: 상세한 기술적 히스토리나 거버넌스 세부 사항은 EVB 프로젝트를 참조하며, 본 프로젝트에는 중복 기술하지 않습니다.
