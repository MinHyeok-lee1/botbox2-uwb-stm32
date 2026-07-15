# 제품 개요 (PRODUCT OVERVIEW): STM32 DW3000 UWB 펌웨어

## 1. 개요 (Overview)
본 프로젝트는 STM32G474RE 마이크로컨트롤러와 DW3000 UWB 트랜시버를 사용하는 Customer delivery용 UWB 펌웨어 패키지입니다. 
STM32CubeMX(IOC Shell)를 기반으로 구축된 02 Product 산출물에서 정화 export되며, 고객 전달에 필요한 런타임 소스와 릴리스 산출물만 유지합니다.

## 2. 프로젝트 역할 및 관계 (Project Roles and Relationships)
- **Customer delivery 프로젝트 (본 저장소)**:
  - 02 Product에서 export된 고객 전달용 런타임 소스와 릴리스 산출물 보존.
  - Product acceptance evidence, 내부 분석, 검증 원본 기록은 02 Product에만 보존하고 본 저장소에는 포함하지 않음.
  - 고객 프로비저닝 및 현장 운영 가이드 제공.
- **02 Product 프로젝트 (Evidence Owner)**:
  - 제품 보드 통합, release acceptance, Product-to-Customer export source 소유.
  - Product acceptance evidence 및 내부 분석 기록 보존.

본 Customer delivery 저장소는 02 Product와 동일한 내부 운영 저장소가 아니며, 고객 전달에 불필요한 내부 증거와 로컬 개발 메타데이터는 포함하지 않습니다. 보드 전용 설정(Clock, FDCAN, pinmux 등)은 02 Product에서 검증된 산출물을 통해 제공됩니다.

## 3. 주요 기능 (Key Capabilities)
- **설정 기반 역할 자동 할당**: `cfg.bin`을 통해 런타임에 TAG 또는 ANCHOR로 동작.
- **SS-TWR (Single-Sided Two-Way Ranging)** 지원.
- **DS-TWR (Double-Sided Two-Way Ranging)** 지원.
- **CAN 거리 정보 송신**: 측정된 거리 데이터를 메인보드로 실시간 전송.
- **제품별 통합 (Product-specific Integration)**: 보드 특성에 맞춘 FDCAN 및 시스템 클럭 최적화.

## 4. 문서 운영 정책 (Documentation Policy)
- **Customer-facing**: 본 저장소의 문서는 고객 전달과 운영에 필요한 범위로 간결하게 유지됩니다.
- **Evidence boundary**: 상세한 acceptance evidence, 내부 분석, 거버넌스 이력은 02 Product에 남기며 본 Customer 저장소에는 중복 기술하지 않습니다.
