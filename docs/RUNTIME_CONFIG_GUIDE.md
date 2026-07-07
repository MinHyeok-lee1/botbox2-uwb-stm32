# STM32-DW3000 UWB 런타임 설정 가이드 (STM32-DW3000 UWB Runtime Configuration Guide)

> [!NOTE]
> **표준 작업 디렉터리**: 이 가이드의 모든 명령은 **저장소 루트(Repository Root)**에서 실행하는 것을 기준으로 합니다.

## 1. 운영 모델 (Operational Model)

### 1-1. 고정형 펌웨어 / 교체형 설정 원칙 (Fixed Firmware / Replaceable Config Principle)
본 UWB 솔루션은 **"고정형 펌웨어 + 교체형 설정"** 원칙으로 설계되어, 현장 운영의 복잡성을 최소화합니다.

- **펌웨어 자산 (`delivery/firmware.bin`)**
  - UWB 통신 엔진, 필터, 역할 디스패치(role dispatch), 지속성 적용 흐름을 포함하는 단일 통합 실행 파일
  - 현장에서는 자주 바뀌지 않는 **고정 자산**
- **설정 자산 (`delivery/configs/*.bin`)**
  - TAG/ANCHOR 역할, PAN ID, Short Address, 앵커 목록 등 장비별 운용 정보를 담는 40바이트 설정 파일
  - 현장에서는 용도에 따라 교체 가능한 **운영 자산**

즉, 현장에서는 **역할별 펌웨어를 따로 굽는 모델이 아니라**, 동일한 `firmware.bin`을 유지한 채 `cfg.bin`만 바꿔 역할과 주소 체계를 전환하는 것이 표준입니다.

### 1-2. 현장 운영 시 변경되는 항목 (What changes in field operation)
현장에서는 아래 항목만 바뀝니다.

- 역할 (`TAG` / `ANCHOR`)
- PAN ID
- Short Address
- 앵커 수 (anchor_count) / 앵커 ID 목록 (anchor_ids)
- 프리셋(preset) 선택

반대로 아래 항목은 현장에서 고정 자산으로 취급합니다.

- UWB 핵심 펌웨어 로직
- 지속성(persistence) 동작
- 런타임 역할 전환 동작
- 바이너리 전용 배포 모델
- 설정 플래시 영역 분리

---

## 2. 설정 필드 (Configuration Fields)

### 2-1. 역할 및 네트워크 (Role & Network)
- **역할 (Role)**
  - `TAG`: 이동 측위 기기
  - `ANCHOR`: 고정 기준 기기
- **PAN ID**
  - 동일 공간 내 UWB 장비 그룹을 묶는 고유 번호
  - 예: `0x2001`
- **Short Address**
  - 해당 PAN 내 각 디바이스의 고유 식별자
- **앵커 (Anchors)**
  - TAG가 통신 대상으로 삼는 앵커 주소 목록
  - 현재 설정 정책상 최대 8개까지 확장 가능

### 2-2. 레인징 및 필터 (Ranging & Filter)
- **레인징 (Ranging)**
  - 현재 검증된 베이스라인은 `SS-TWR` (기본) 및 `DS-TWR` (1:1, 1:3 검증 완료)
  - 운영 기준에서는 목적에 따라 SS-TWR 또는 DS-TWR 선택이 가능합니다.
- **필터 (Filter)**
  - `WEAK`, `NORMAL`, `STRONG`
  - 현재 실보드 베이스라인은 검증된 프리셋 기준 사용 권장
- **통신 모드 (Comm Mode)**
  - 현재 검증된 베이스라인은 `ANYCAST`
  - 운영 기준에서는 임의 변경보다 프리셋 유지를 권장합니다.

---

## 3. 표준 프리셋 (Standard Presets)

`delivery/configs/` 경로에는 사전 검증이 완료된 설정 파일들이 포함됩니다.

`cfg.bin` is the runtime configuration input. Use only validated configuration files included in the customer delivery package; internal policy and host-validation records are maintained outside this repository.

### 3-1. 골든 프리셋 (Golden Presets)
- `tag_a1001_ss_3a_p2001.bin`
- `anc_a2001_ss_p2001.bin`

### 3-2. 운영용 프리셋 (Operational Presets)
검증 완료된 1 TAG + 3 ANCHOR 세트:

- `tag_a1001_ss_3a_p2001.bin`
- `anc_a2001_ss_p2001.bin`
- `anc_a2002_ss_p2001.bin`
- `anc_a2003_ss_p2001.bin`

### 3-3. 현재 검증된 토폴로지 (Current validated topology)
현재 실보드 기준 검증된 토폴로지는 아래와 같습니다.

- **PAN ID**: `0x2001`
- **TAG**: `0x1001`
- **ANCHOR 1**: `0x2001`
- **ANCHOR 2**: `0x2002`
- **ANCHOR 3**: `0x2003`

---

## 4. 프로비저닝 절차 (Provisioning Procedures)

### 4-1. 검증된 4개 보드 레퍼런스 매핑 (Validated 4-Board Reference Mapping)
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

### 4-2. 전체 프로비저닝 (검증된 명령문) (Full Provisioning)
```powershell
.\tools\provision.ps1 -sn 002E002B3234511137333934 -fw delivery\firmware.bin -cfg delivery\configs\anc_a2001_ss_p2001.bin
.\tools\provision.ps1 -sn 0027003F3234510F37333934 -fw delivery\firmware.bin -cfg delivery\configs\anc_a2002_ss_p2001.bin
.\tools\provision.ps1 -sn 003D002A3234511137333934 -fw delivery\firmware.bin -cfg delivery\configs\anc_a2003_ss_p2001.bin
.\tools\provision.ps1 -sn 004D003D3234510F37333934 -fw delivery\firmware.bin -cfg delivery\configs\tag_a1001_ss_3a_p2001.bin
```

### 4-3. 일반 운영 시나리오 (General Operation Scenarios)

#### 신규 디바이스 완전 배포

```powershell
.\tools\provision.ps1 -sn <SN> -fw delivery\firmware.bin -cfg delivery\configs\tag_a1001_ss_3a_p2001.bin
```

#### 설정(Config)만 변경

```powershell
.\tools\provision.ps1 -sn <SN> -cfg delivery\configs\anc_a2001_ss_p2001.bin
```

#### 펌웨어(Firmware)만 업데이트

```powershell
.\tools\provision.ps1 -sn <SN> -fw delivery\firmware.bin
```

---

## 5. 런타임 로그 해석 (Runtime Interpretation)

### 5-1. 앵커 측 (Anchor-side)

#### 링크 상태 가시성 (Link-state visibility)

* `[ANC][LINK] lost: no matched poll for ... ms`
* `[ANC][LINK] lost: still no matched poll for ... ms`
* `[ANC][LINK] resumed: matched poll received`

의미:
* ANCHOR는 살아 있으나 지정된 피어(peer) TAG의 일치하는 poll이 일정 시간 들어오지 않음을 뜻합니다.
* 현장 운영자는 "보드 멈춤"과 "링크 단절"을 구분할 수 있습니다.

#### 전력 상태 가시성 (Power-state visibility)

* `[ANC][PWR] enter low-power listen: ...`: 1분 이상 무응답 시 저전력 slow-listen 진입
* `[ANC][RECOV] enter LOW_POWER_LISTEN: tick=... listen_win=250 sleep_win=2250`: slow-listen 진입 확인용 기본 운영 로그
* `[ANC][PWR] wake: periodic check`: 저전력 모드 중 주기적(10초) 하트비트(heartbeat)
* `[ANC][PWR] exit low-power listen: matched poll received`: 저전력 탈출 및 복구

#### DS-TWR 거리 로그 (DS-TWR distance log)

* `[ANC][DS] src=0x... dist=...mm`: DS-TWR 모드 시 ANCHOR 측 거리 출력

#### `[ANC][STAT]` 상태별 출력 정책

운영 통계 로그(`[ANC][STAT]`)는 ANCHOR 상태에 따라 다르게 출력됩니다:

| ANCHOR 상태 | `[ANC][STAT]` 출력 주기 |
|---|---|
| `ACTIVE` | 약 1초 |
| `LINK_LOST` | 약 10초 |
| `LOW_POWER_LISTEN` | 완전 억제 |

`LOW_POWER_LISTEN`에서 `[ANC][STAT]`를 억제하는 이유:
저전력 상태에서 `[ANC][PWR] wake`, `[ANC][PWR] exit low-power listen`, `[ANC][LINK] resumed` 등 운영자에게 중요한 로그가 `[ANC][STAT]` 반복 출력에 묻히지 않도록 하기 위함입니다.

#### `rx_affrej` 해석 (rx_affrej interpretation)

1 TAG + 3 ANCHOR 구조에서는 `rx_affrej` 증가를 단독으로 문제로 해석하지 않습니다.

정상 해석 조건:
* `rx_ok` 증가
* `tx_ok` 증가
* `bad_pan=0`
* `bad_dst=0`

즉, 현재 토폴로지에서는 `rx_affrej`가 **엿듣기(overhearing) + 하드웨어 필터 거부(hardware filter reject)**의 부산물일 수 있습니다.

### 5-2. 태그 측 (TAG-side)

#### 멀티 앵커 요약 로그 (Multi-anchor summary log)

* `[TAG][MULTI] ok=... A0=... A1=... A2=...`

의미:
* 현재 전체 스캔에서 몇 개의 앵커와 유효한 레인징이 성립했는지 요약합니다.
* 현재 검증된 베이스라인에서는 `ok=3/3`가 반복되면 정상 운영 상태로 해석합니다.

#### 전체 스캔 페이싱 로그 (Full-scan pacing log)

* `[TAG][CYCLE] air=... wait=... target=... overrun=...`

의미:
* `air`: 실제 전체 스캔 활성 시간 (full scan active time)
* `wait`: 페이싱 지연 (pacing delay)
* `target`: 목표 전체 스캔 주기 (target full-scan period)
* `overrun=1`: 현재 목표 Hz를 못 맞춘 포화(saturation) 상태

#### 현재 검증된 베이스라인 (Current validated baseline)

* 현재 3-anchor 기준 안정된 베이스라인은 **6 Hz 전체 스캔(full-scan)**입니다.
* 최근 실보드 로그에서도 `[TAG][MULTI] ok=3/3`와 `[TAG][CYCLE] overrun=0`가 반복적으로 확인되었습니다.
* 즉, 3-anchor / 6 Hz 베이스라인은 반복 재현 가능한 운영 기준선으로 해석 가능합니다.

---

## 6. 앵커 수 스케일링 — 초기 운영 규칙 (Anchor Count Scaling — Initial Operating Rule)

현재 3-anchor 실측 로그 기준:
* 전체 스캔 활성 시간(full scan active time) ≈ **144 ms**
* 앵커 1개당 실제 활성 슬롯(active slot) ≈ **48 ms**

실무용 근사치:
* `T_target(N) ≈ 55 * N ms`
* `f_scan(N) ≈ 1000 / (55 * N)`
* 단순 운영 근사치: `f_scan(N) ≈ 18 / N`

### 권장 초기 전체 스캔 Hz (Recommended Initial Full-Scan Hz)

| 앵커 수 (N) | 권장 초기 전체 스캔 Hz |
| ---------------- | -------------------------------: |
| 1                |                       15 ~ 18 Hz |
| 2                |                             9 Hz |
| 3                |                             6 Hz |
| 4                |                         4 ~ 5 Hz |
| 5                |                         3 ~ 4 Hz |
| 6                |                             3 Hz |
| 8                |                             2 Hz |

> 위 표는 **현재 코드 구조 / 로그량 / 품질 게이트 조건 기준의 초기 운영값**입니다. 최종값은 반드시 `[TAG][CYCLE] air / wait / overrun` 로그를 보고 현장에서 확정해야 합니다.

---

## 7. Sleep / Wake 정책 및 로깅 가시성 (Sleep / Wake Policy & Logging Visibility)

ANCHOR의 저전력 정책 및 로그 가시성 정책은 하드웨어 검증이 완료된 상태입니다.

### 상태 전환 정책
* `ACTIVE` → `LINK_LOST`: 일치하는 poll이 5초 이상 없을 때
* `LINK_LOST` → `LOW_POWER_LISTEN`: 추가로 1분 이상 무응답 지속 시
* `LOW_POWER_LISTEN`: accepted slow-listen window (250ms 수신 / 2250ms 수면)
* 복구: 어떤 비-ACTIVE 상태에서든 일치하는 poll 수신 시 즉시 `ACTIVE`로 복귀

### 운영자 로그
* `[ANC][LINK] lost: no matched poll for ... ms`: 최초 링크 단절
* `[ANC][LINK] lost: still no matched poll for ... ms`: 단절 유지 (5초 주기)
* `[ANC][LINK] resumed: matched poll received`: 통신 복구
* `[ANC][PWR] enter low-power listen: ...`: 저전력 진입
* `[ANC][RECOV] enter LOW_POWER_LISTEN: tick=... listen_win=250 sleep_win=2250`: 저전력 slow-listen 진입 확인
* `[ANC][PWR] wake: periodic check`: 저전력 중 하트비트 (10초 주기)
* `[ANC][PWR] exit low-power listen: matched poll received`: 저전력 탈출

### `[ANC][STAT]` 출력 정책
운영 통계 로그(`[ANC][STAT]`)는 ANCHOR 상태에 따라 다르게 출력됩니다:

| ANCHOR 상태 | `[ANC][STAT]` 출력 주기 |
|---|---|
| `ACTIVE` | 약 1초 |
| `LINK_LOST` | 약 10초 |
| `LOW_POWER_LISTEN` | 완전 억제 |

이 정책으로 인해 저전력 상태에서는 `[ANC][PWR]` 로그가 `[ANC][STAT]`에 묻히지 않고 정상적으로 읽힙니다.

---

## 8. 문제 해결 (Troubleshooting)

### 에러: `Multiple probes found. Please specify -sn <Serial>.`
* **원인**: PC에 STM32 디바이스가 2대 이상 연결되어 있어 자동 타겟팅이 차단됨
* **해결**: `-sn [시리얼]` 옵션 명시

### 에러: `No ST-Link probe found.`
* **해결**: 보드 연결, 케이블, 전원 상태 확인

### 에러: `Config size (... bytes) != expected 40 bytes.`
* **원인**: 유효하지 않은 cfg.bin
* **해결**: `delivery/configs/`의 정식 파일 사용

### 에러: `STM32CubeProgrammer not found or not in PATH.`
* **해결**: STM32CubeProgrammer 설치 후 PATH 추가 또는 `STM32_CLI` 환경변수 설정

### 현상: Anchor에서 `rx_affrej`가 많이 증가함
* **해석**: 1 TAG + 3 ANCHOR 구조에서는 엿듣기(overhearing) 기반 거부(reject)일 수 있음
* **정상 판정 조건**: `rx_ok`, `tx_ok` 증가 + `bad_pan=0`, `bad_dst=0`

### 현상: TAG에서 `overrun=1`이 반복됨
* **해석**: 현재 앵커 수 / 로그 비용 / 목표 Hz 조합에서 포화(saturation) 상태임
* **해결 방향**:
  * 전체 스캔 Hz를 낮춤
  * 먼저 `18 / N` 근사치로 재시작
  * `[TAG][CYCLE]` 로그로 운영 가능값을 다시 확정

---

## 9. 비상시 GUI 폴백 (Emergency GUI Fallback)

스크립트 실행이 불가능한 극한 상황에서만 GUI를 사용하십시오:

1. **펌웨어(Firmware)**: `delivery/firmware.bin` → 주소 `0x08000000`
2. **설정(Config)**: 대상 `cfg.bin` → 주소 `0x0807F800`

> 주소 오입력 시 부팅이 불가능할 수 있으므로 표준 스크립트 사용을 우선합니다.

---

## 10. 이 가이드의 범위와 제외 사항 (What this guide covers vs what it does not)

### 이 가이드가 다루는 내용
* 고정형 펌웨어 + cfg.bin 운영 철학
* 검증된 토폴로지
* 프로비저닝(provisioning) 절차
* 운영자 관점의 런타임 로그 해석
* 앵커 수 증가 시 초기 운영값

### 이 가이드가 다루지 않는 내용
* 거리 정확도 / 안정도 보정값
* 하드웨어 수정 이후의 최종 튜닝 결과
* 앵커 수 스케일링 최종 운영값

이 항목들은 현재 프로젝트에서 **후속 과제**로 진행 중이며, 최종 확정 전까지는 고객 전달 범위에 포함된 최신 안내 문서를 확인해야 합니다.
