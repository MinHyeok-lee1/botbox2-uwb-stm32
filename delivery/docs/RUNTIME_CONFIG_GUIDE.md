# STM32-DW3000 UWB Runtime Configuration Guide

> [!NOTE]
> **Canonical Working Directory**: All commands in this guide are designed to be run from the **Repository Root**.

## 1. 운영 개념 (Operational Overview)

### 제품 운영 철학
본 UWB 솔루션은 **"고정형 펌웨어 + 교체형 설정"** 원칙으로 설계되어, 현장 운영의 복잡성을 최소화합니다.

- **Firmware Asset (`delivery/firmware.bin`)**: UWB 통신 엔진, 필터, 링커 등 핵심 논리를 포함하는 단일 통합 실행 파일입니다. 현장에 한 번 배포된 후에는 잦은 변경이 발생하지 않는 "고정 자산"입니다.
- **Config Asset (`delivery/configs/*.bin`)**: 디바이스의 역할(TAG/ANCHOR), 물리 주소(PAN ID, Short Addr), 대상 앵커 목록 등 현장 맞춤형 데이터를 담고 있는 40바이트의 설정 파일입니다. "교체 가능 자산"입니다.

**현장 운영의 핵심**:  
단순히 장비의 역할을 바꾸거나 주소를 재할당하려면 펌웨어 전체를 재설치하거나 다시 빌드할 필요가 없습니다. 용도에 맞는 `cfg.bin` 파일 하나만 교체(프로비저닝)하면 즉시 변경 사항이 적용됩니다.

---

## 2. 설정 필드 이해 (Configuration Fields)

### 역할 및 주소 (Role & Network)
- **Role (역할)**: `TAG`(이동 측위 기기) 또는 `ANCHOR`(고정 기준 기기)
- **PAN ID**: 동일 공간 내 UWB 장비 그룹을 묶는 고유번호 (예: `0x2001`)
- **Short Address**: 해당 그룹 내 각 디바이스의 고유 식별자
- **Anchors**: 기기가 통신할 대상 앵커들의 주소 목록 (최대 8개)

### 통신 및 필터 (Ranging & Filter)
- **Ranging**: 측위 프로토콜 (현재 `SS-TWR` 기본, `DS-TWR` 1:1 및 1:3 셋업 검증 완료)
- **Filter**: 노이즈 억제 및 필터링 강도 (`WEAK`, `NORMAL`, `STRONG`)
- **Comm Mode**: 통신 다중화 모드 (현재 버전 `ANYCAST` 고정 권장)

> **표준 배포 프리셋 정규 목록 (Standard Presets)**  
> `delivery/configs/` 경로에는 사전 검증이 완료된 표준 설정 파일들이 동봉되어 있습니다.
> - **Golden Presets**
>   - `tag_a1001_ss_3a_p2001.bin`
>   - `anc_a2001_ss_p2001.bin`
> - **Operational Presets** (검증 완료된 1-TAG, 3-ANCHOR 세트)
>   - `tag_a1001_ss_3a_p2001.bin`
>   - `anc_a2001_ss_p2001.bin`
>   - `anc_a2002_ss_p2001.bin`
>   - `anc_a2003_ss_p2001.bin`

---

## 3. 현장 운영 가이드 (Provisioning Procedures)

### 3-1. 4-Board Reference Mapping (Validated Session)
- **TAG**
  - ST-Link SN: `004D003D3234510F37333934`
  - COM: `COM13`
  - cfg: `delivery/configs/tag_a1001_ss_3a_p2001.bin`
- **ANCHOR 1**
  - ST-Link SN: `002E002B3234511137333934`
  - COM: `COM10`
  - cfg: `delivery/configs/anc_a2001_ss_p2001.bin`
- **ANCHOR 2**
  - ST-Link SN: `0027003F3234510F37333934`
  - COM: `COM12`
  - cfg: `delivery/configs/anc_a2002_ss_p2001.bin`
- **ANCHOR 3**
  - ST-Link SN: `003D002A3234511137333934`
  - COM: `COM11`
  - cfg: `delivery/configs/anc_a2003_ss_p2001.bin`

### 3-2. Full Provisioning (Validated Copy-Paste)
```powershell
.\tools\provision.ps1 -sn 004500403234511137333934 -fw delivery\firmware.bin -cfg delivery\configs\anc_a2001_ss_p2001.bin
.\tools\provision.ps1 -sn 0027003F3234510F37333934 -fw delivery\firmware.bin -cfg delivery\configs\anc_a2002_ss_p2001.bin
.\tools\provision.ps1 -sn 003D002A3234511137333934 -fw delivery\firmware.bin -cfg delivery\configs\anc_a2003_ss_p2001.bin
.\tools\provision.ps1 -sn 004D003D3234510F37333934 -fw delivery\firmware.bin -cfg delivery\configs\tag_a1001_ss_3a_p2001.bin
```

### 3-3. 일반 운영 시나리오
#### 신규 디바이스 완전 배포
```powershell
.\tools\provision.ps1 -sn <SN> -fw delivery\firmware.bin -cfg delivery\configs\tag_a1001_ss_3a_p2001.bin
```

#### Config-Only 변경
```powershell
.\tools\provision.ps1 -sn <SN> -cfg delivery\configs\anc_a2001_ss_p2001.bin
```

#### Firmware-Only 업데이트
```powershell
.\tools\provision.ps1 -sn <SN> -fw delivery\firmware.bin
```

---

## 4. 현장 로그 해석 (Operator Log Interpretation)

### 4-1. Anchor-side
#### Link-state Visibility
- `[ANC][LINK] lost: no matched poll for ... ms`
- `[ANC][LINK] lost: still no matched poll for ... ms`
- `[ANC][LINK] resumed: matched poll received`

의미:
- ANCHOR가 정상 동작 중이지만, 지정된 peer TAG의 matched poll이 일정 시간 들어오지 않음을 의미합니다.
- 현장 오퍼레이터는 “보드 멈춤”과 “링크 단절”을 구분할 수 있습니다.

#### Power-state Visibility
- `[ANC][PWR] enter low-power listen: ...`: 1분 이상 무응답 시 저전력(10% duty-cycle) 진입
- `[ANC][PWR] wake: periodic check`: 저전력 모드 중 주기적(10초) heartbeat
- `[ANC][PWR] exit low-power listen: matched poll received`: 저전력 탈출 및 복구

#### DS-TWR Distance Log
- `[ANC][DS] src=0x... dist=...mm`: DS-TWR 모드 시 ANCHOR 측 거리 출력

#### `[ANC][STAT]` 상태별 출력 정책
운영 통계 로그(`[ANC][STAT]`)는 ANCHOR 상태에 따라 다르게 출력됩니다:
- `ACTIVE`: 약 1초 주기로 출력
- `LINK_LOST`: 약 10초 주기로 throttle
- `LOW_POWER_LISTEN`: 완전 억제

`LOW_POWER_LISTEN` 상태에서 `[ANC][STAT]`을 완전히 억제하는 이유:
저전력 상태에서는 `[ANC][PWR] wake: periodic check`, `[ANC][PWR] exit low-power listen`, `[ANC][LINK] resumed` 등 오퍼레이터에게 중요한 상태 로그가 반복되는 통계 로그(`[ANC][STAT]`)에 묻히지 않고 현장에서 명확히 읽히도록 하기 위함입니다.

#### `rx_affrej` 해석
- 1 TAG + 3 ANCHOR 구조에서는 `rx_affrej` 증가가 **정상적인 overhearing / address-filter reject**일 수 있습니다.
- `rx_ok`, `tx_ok`가 같이 증가하고 `bad_pan=0`, `bad_dst=0`이면 `rx_affrej` 단독 증가는 버그로 보지 않습니다.

### 4-2. TAG-side
#### Full-scan Pacing Log
- `[TAG][CYCLE] air=... wait=... target=... overrun=...`

의미:
- `air`: 실제 full scan active time
- `wait`: pacing delay
- `target`: 목표 full-scan period
- `overrun=1`: 현재 target Hz를 못 맞추는 saturation 상태

#### Current validated baseline
- 3-anchor 기준 stable baseline은 **6 Hz full-scan**
- 7 Hz 이상에서는 `overrun=1`, `wait=2`가 반복되어 현재 구조상 saturation으로 해석함

---

## 5. Anchor Count 증가 시 초기 Hz 정책

현재 3-anchor 실측 로그 기준:
- full scan active time ≈ **144 ms**
- anchor 1개당 실전 active slot ≈ **48 ms**

실무용 근사:
- `T_target(N) ≈ 55 * N ms`
- `f_scan(N) ≈ 1000 / (55 * N)`
- 단순 운영 근사:
  - `f_scan(N) ≈ 18 / N`

### 초기 추천표
| Anchor Count (N) | Recommended Initial Full-Scan Hz |
|---|---:|
| 1 | 15 ~ 18 Hz |
| 2 | 9 Hz |
| 3 | 6 Hz |
| 4 | 4 ~ 5 Hz |
| 5 | 3 ~ 4 Hz |
| 6 | 3 Hz |
| 8 | 2 Hz |

> 최종값은 반드시 `[TAG][CYCLE] air / wait / overrun` 로그로 다시 확정하십시오.

---

## 6. 트러블슈팅 (Troubleshooting)

### 에러: `Multiple probes found. Please specify -sn <Serial>.`
- **원인**: PC에 STM32 디바이스 2대 이상이 연결되어 있어 자동 타겟팅이 차단됨
- **해결**: `-sn [시리얼]` 옵션 명시

### 에러: `No ST-Link probe found.`
- **해결**: 보드 연결, 케이블, 전원 상태 확인

### 에러: `Config size (... bytes) != expected 40 bytes.`
- **원인**: 유효하지 않은 cfg.bin
- **해결**: `delivery/configs/`의 정식 파일 사용

### 에러: `STM32CubeProgrammer not found or not in PATH.`
- **해결**: STM32CubeProgrammer 설치 후 PATH 추가 또는 `STM32_CLI` 환경변수 설정

---

## 7. 비상 수동 복구 경로 (Emergency GUI Fallback)
스크립트 실행이 불가능한 극한 상황에서만 GUI 사용:

1. **Firmware**: `delivery/firmware.bin` → 주소 `0x08000000`
2. **Config**: 대상 `cfg.bin` → 주소 `0x0807F800`

> 주소 오입력 시 부팅 불가 가능성이 있으므로 표준 스크립트 사용을 우선합니다.
