# STM32 DW3000 IOC-Shell (현재 상태: 통합 완료)

> [!IMPORTANT]
> **Start here — Customer Delivery**
> - 역할: 02 Product에서 정화 export된 Customer delivery 저장소입니다.
> - 금지: Product acceptance evidence, 내부 분석 문서, 로컬 개발 메타데이터를 이 Customer 저장소로 옮기지 마십시오.
> - Product runtime 변경과 acceptance evidence는 02 Product 저장소에서 소유하며, 이 저장소는 전달 가능한 산출물만 유지합니다.
> - 먼저 [AGENTS.md](AGENTS.md), 이어서 [docs/COMMAND_INDEX.md](docs/COMMAND_INDEX.md)를 읽으십시오.
> - Customer 사용자는 [docs/PRODUCT_QUICK_START.md](docs/PRODUCT_QUICK_START.md)와 이 저장소의 전달 문서만 참고하십시오.

## Documentation Entry

Start here for the Customer delivery repository:

- Repository guidance: see `docs/COMMAND_INDEX.md` and `docs/OPERATOR_QUICK_COMMANDS.md` for onboarding.
- Command index: `docs/COMMAND_INDEX.md`.
- Operator workflow: `docs/OPERATOR_QUICK_COMMANDS.md`.
- Product overview and setup for Customer delivery: `docs/PRODUCT_OVERVIEW.md`, then `docs/PRODUCT_QUICK_START.md`.
- CAN contract: `docs/PRODUCT_CAN_INTERFACE.md` and `docs/contracts/UWB_CAN_PACKET_CONTRACT.md`.

Customer delivery checks:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\check_delivery.ps1
```

This Customer repository contains the sanitized delivery source and release assets exported from 02 Product. Product acceptance evidence stays in 02 Product, not in this Customer repository.

## Current Release Status

- Current Product baseline: `v1.0.0-product-accepted`
- This repo is the sanitized Customer delivery snapshot exported from 02 Product.
- Product acceptance evidence and internal analysis stay in 02 Product, not in this Customer repository.
- Customer release source: clean export from 02 Product.
- 2026-07-15 delivery status: CH9 DW3000 recovery fixes have been propagated into this Customer delivery snapshot as sanitized runtime source and release assets. Detailed Product validation records remain in 02 Product.
- 현재 Customer parity는 02 Product와 firmware binary SHA가 항상 byte-identical임을 뜻하지 않습니다. 공유 source/critical fix parity, Customer delivery manifest self-integrity, Product-only artifact exclusion을 기준으로 검증합니다.

### Release Firmware Assets

The release package includes the following explicit firmware assets depending on operating profile:

- `firmware_product_ch5.bin`
- `firmware_product_ch9.bin`
- `firmware_rxdiag_ch5.bin`
- `firmware_rxdiag_ch9.bin`

02 Product의 `firmware_product_ch9_tdma_optimal.bin`은 Product-only opt-in artifact이며 Customer canonical delivery에 포함하지 않습니다.

Runtime role, PAN ID, short address, and anchor topology are still controlled by `cfg.bin` at the reserved configuration flash area.

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
본 프로젝트는 **고정 펌웨어(Golden Firmware)** 모델 및 운영 프로필별 release assets 구조를 따릅니다:
1. **명시적 채널/프로필 펌웨어:** 현재 release assets는 `firmware_product_ch5.bin`, `firmware_product_ch9.bin`, `firmware_rxdiag_ch5.bin`, `firmware_rxdiag_ch9.bin`으로 구성됩니다. legacy 공통 `firmware.bin` / `firmware_rxdiag.bin`은 현재 Customer delivery asset이 아닙니다.
2. **cfg.bin:** 런타임에 보드의 역할(TAG/ANCHOR), ID 및 레인징 파라미터를 결정하는 소형(40바이트) 설정 파일.

---
*최종 업데이트: 2026-07-15*
*

### CH9 5Hz Multipath-Tolerant Conservative Profile

- CH9 TAG ranging rate changed to 5Hz for higher temporal resolution.
- CH9 uses 55ms anchor slots and 6.5ms Slot 2/3 guards; including the 3ms minimum gap, the 200ms period retains 19ms margin.
- CH9 anchor-side filtering focuses on output slew-rate stability rather than aggressive multipath rejection.
- Multipath/NLOS-like measurements remain candidates when they form a plausible trend.
- CH5 timing and filter profiles remain unchanged.
- Define `UWB_CH9_FILTER_DEBUG=1` to print the active CH9 filter profile once at initialization.

### CH9 Extreme-Only Multipath Filtering Experiment

- CH9 keeps multipath/NLOS-like measurements as candidates when the distance and RX diagnostics are physically valid.
- Timeout, RX/frame errors, malformed diagnostics, and invalid, zero, negative, NaN/Inf, or over-range distances remain hard rejects.
- Final output stability is controlled primarily by the 360mm/sample LPF slew limit rather than aggressive quality or Kalman rejection.
- CH9 soft-accepted measurements use a minimum Kalman `los_conf` of 0.45; CH5 behavior remains unchanged.
- Define `UWB_CH9_FILTER_DEBUG=1` to enable rate-limited `[UWB][CH9][QUAL]` policy logs.

### CH9 Diagnostic Glitch Hold

- CH9 treats one-off `diag_insane` frames as diagnostic glitches instead of immediate communication loss.
- The first two consecutive glitches resend the last valid distance for up to 600ms without updating LPF/KF state.
- Three consecutive or persistent diagnostic failures fall back to the existing communication-lost `-1` behavior.
- A successful distance update clears the per-anchor glitch state; CH5 behavior remains unchanged.
