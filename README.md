# STM32 DW3000 IOC-Shell (현재 상태: 통합 완료)

## 1. 개요 (Overview)
본 저장소는 STM32G474RE + DW3000 플랫폼을 위한 통합 UWB 레인징 펌웨어를 포함하고 있습니다. 깨끗한 CubeMX 생성 "IOC Shell"을 기반으로 구축되었으며, 참조 저장소의 UWB 애플리케이션 로직을 성공적으로 이식하여 고안정성 레인징을 검증 완료했습니다.

## 2. 현재 프로젝트 상태 (Current Project Status)
- **베이스라인 (Baseline):** `main` 브랜치와 통합 완료.
- **하드웨어 추상화 (Hardware Abstraction):** `.ioc` 파일을 통해 관리 (STM32CubeMX 권위).
- **핵심 로직 (Core Logic):** 다중 앵커 UWB 런타임 (SS-TWR 및 DS-TWR 지원).
- **프로비저닝 (Provisioning):** `0x0807F800` 위치의 `cfg.bin`을 통한 설정 기반 역할 할당 (TAG/ANCHOR).

## 3. 프로젝트 구조 및 거버넌스 (Project Structure & Governance)
본 저장소는 **실제 제품 UWB 프로젝트**입니다. 참조용 업스트림(EVB) 프로젝트와의 협업을 통해 기능 승격 및 계약 정합성을 유지하며, 보드 통합 및 제품 수용을 담당합니다.

### 독본 순서 (Recommended Reading Flow)
1. [PRODUCT_OVERVIEW.md](docs/PRODUCT_OVERVIEW.md) (프로젝트 개요)
2. [PRODUCT_QUICK_START.md](docs/PRODUCT_QUICK_START.md) (빌드 및 프로비저닝)
3. [PRODUCT_CAN_INTERFACE.md](docs/PRODUCT_CAN_INTERFACE.md) (CAN 프로토콜 요약)
4. [PRODUCT_PROVISIONING_GUIDE.md](docs/PRODUCT_PROVISIONING_GUIDE.md) (고정 펌웨어 모델 안내)

> [!NOTE]
> 상세 내부 개발 이력과 심층 거버넌스는 별도 내부 기술 문서에서 관리합니다.

## 4. 거버넌스 원칙 (Governance Principles)
- **일치 필수 (계약 사항)**: SS/DS-TWR 역할 세만틱, TAG/ANCHOR 소유권, CAN 패킷 계약/ID 매핑, 거리 단위, 체크섬 규칙.
- **의도적 제품 특화**: 시스템 클럭 설정, FDCAN 라우팅, GPIO 핀믹스, 메인보드 CAN 통합.
- **품질 보증**: 모든 기능은 실제 제품 보드에서의 수용 테스트(Acceptance Test)를 거쳐 확정됩니다.

상세 기술 계약은 [UWB_CAN_PACKET_CONTRACT.md](docs/contracts/UWB_CAN_PACKET_CONTRACT.md)를 참조하십시오.

## 5. 검증된 기능 (Validated Capabilities)
- **다중 앵커 레인징:** 6Hz 베이스라인에서 안정적인 1:3 DS-TWR 레인징 확인.
- **프로토콜 안정성:** 에이징 테스트(Burn-in) 결과 99.9% 이상의 레인징 성공률 기록.
- **타이밍 무결성:** 100% 주기 안정성 (태스크 오버런 발생 제로).
- **빌드 시스템:** 정의된 CMake 프리셋을 사용한 깨끗한 빌드.

## 6. 하드웨어 설정 (IOC Authority)
- **SPI1:** DW3000 통신 (`PA4`=CS, `PA15`=RESET).
- **LPUART1:** 디버그 로깅 (115200 8N1).
- **LEDs:** 시스템 상태 표시기 (`PA8`=LED_RIGHT).

## 7. 배포 모델 (Deployment Model)
본 프로젝트는 **고정 펌웨어(Golden Firmware)** 모델을 따릅니다:
1. **firmware.bin:** 모든 보드에 배포되는 단일 공통 바이너리.
2. **cfg.bin:** 런타임에 보드의 역할(TAG/ANCHOR), ID 및 레인징 파라미터를 결정하는 소형(40바이트) 설정 파일.

---
*최종 업데이트: 2026-04-29*
*