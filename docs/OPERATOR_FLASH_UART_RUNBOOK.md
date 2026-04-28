# 현장 배포 전 Flash / UART 확인 Runbook

## 목적

이 문서는 현장 배포 승인 전에 운영자가 `delivery/` 패키지를 이용해 firmware flash, cfg 적용, UART boot/runtime log 확인을 수행할 때 따를 절차를 정리한다.

이 문서는 절차 안내용 runbook이다. 이 문서 작성 단계에서는 실제 flash, build, rebuild, hardware test, validation run을 수행하지 않는다.

## 적용 범위

- 저장소 루트(Repository Root)에서 실행한다.
- 표준 firmware artifact는 `delivery/firmware.bin`이다.
- 표준 cfg artifact는 `delivery/configs/*.bin`이다.
- Windows 표준 wrapper는 `tools/provision.ps1`이다.
- UART 확인 로그는 `artifacts/operator_delivery/` 아래에 저장한다.

## 사전 확인

1. 저장소 루트에서 작업한다.
2. `delivery/firmware.bin`과 `delivery/firmware.hex` freshness를 확인한다.

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\check_delivery.ps1
```

기대 결과:

- `delivery/firmware.bin`: `[OK]`
- `delivery/firmware.hex`: `[OK]`

둘 중 하나라도 `[WARNING]`이면 flash를 시작하지 않는다.

3. `delivery/configs/`에 아래 cfg 파일이 있는지 확인한다.

- `delivery/configs/tag_a1001_ss_3a_p2001.bin`
- `delivery/configs/anc_a2001_ss_p2001.bin`
- `delivery/configs/anc_a2002_ss_p2001.bin`
- `delivery/configs/anc_a2003_ss_p2001.bin`

## 보드 / cfg 매핑

| 역할 | ST-Link SN | UART COM | cfg |
|---|---|---|---|
| TAG | `004D003D3234510F37333934` | `COM13` | `delivery/configs/tag_a1001_ss_3a_p2001.bin` |
| ANCHOR 1 | `002E002B3234511137333934` | `COM10` | `delivery/configs/anc_a2001_ss_p2001.bin` |
| ANCHOR 2 | `0027003F3234510F37333934` | `COM12` | `delivery/configs/anc_a2002_ss_p2001.bin` |
| ANCHOR 3 | `003D002A3234511137333934` | `COM11` | `delivery/configs/anc_a2003_ss_p2001.bin` |

## Flash / cfg 적용 명령

PowerShell 실행 정책 때문에 `.ps1` 직접 실행이 막히면 아래처럼 `-ExecutionPolicy Bypass` 형식을 사용한다.

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\provision.ps1 -sn 002E002B3234511137333934 -fw delivery\firmware.bin -cfg delivery\configs\anc_a2001_ss_p2001.bin
powershell -ExecutionPolicy Bypass -File .\tools\provision.ps1 -sn 0027003F3234510F37333934 -fw delivery\firmware.bin -cfg delivery\configs\anc_a2002_ss_p2001.bin
powershell -ExecutionPolicy Bypass -File .\tools\provision.ps1 -sn 003D002A3234511137333934 -fw delivery\firmware.bin -cfg delivery\configs\anc_a2003_ss_p2001.bin
powershell -ExecutionPolicy Bypass -File .\tools\provision.ps1 -sn 004D003D3234510F37333934 -fw delivery\firmware.bin -cfg delivery\configs\tag_a1001_ss_3a_p2001.bin
```

각 명령은 빨간색 오류 없이 완료되어야 한다. 실패한 보드는 다음 보드로 진행하지 말고 실패 조건을 기록한다.

## UART 확인 기준

UART는 115200 baud 기준으로 확인한다.

각 보드에서 확인할 공통 기준:

- `[UWB][CFG] Load success`가 출력된다.
- 의도한 역할이 출력된다.
  - TAG: `ROLE: TAG`
  - ANCHOR: `ROLE: ANCHOR`
- PAN / address / cfg가 의도한 보드 매핑과 어긋나지 않는다.
- 반복적인 hard fault, reset loop, config load failure가 없다.

TAG에서 확인할 기준:

- `[TAG][MULTI] ok=...` 로그가 출력된다.
- 정상 운영에서는 `ok=3/3`가 반복되는지 확인한다.
- `[TAG][CYCLE] air=... wait=... target=... overrun=...` 로그가 출력된다.
- 배포 승인 전 확인에서는 반복적인 `overrun=1`을 정상으로 간주하지 않는다.

ANCHOR에서 확인할 기준:

- `[ANC][LINK] lost`, `[ANC][LINK] resumed` 로그가 상황에 맞게 출력된다.
- `[ANC][PWR] enter`, `[ANC][PWR] wake`, `[ANC][PWR] exit` 로그는 전력 상태 해석용으로만 확인한다.
- DS-TWR 사용 시 `[ANC][DS] src=0x... dist=...mm` 로그가 출력될 수 있다.
- `bad_pan=0`, `bad_dst=0` 조건을 우선 확인한다.
- 1 TAG + 3 ANCHOR 구조에서는 `rx_affrej` 증가만으로 실패로 판정하지 않는다.

## 로그 저장 위치

루트 디렉터리에 로그 파일을 만들지 않는다.

권장 저장 위치:

```text
artifacts/operator_delivery/YYYYMMDD-HHMM/
```

권장 파일명:

```text
artifacts/operator_delivery/YYYYMMDD-HHMM/tag_COM13.log
artifacts/operator_delivery/YYYYMMDD-HHMM/anchor1_COM10.log
artifacts/operator_delivery/YYYYMMDD-HHMM/anchor2_COM12.log
artifacts/operator_delivery/YYYYMMDD-HHMM/anchor3_COM11.log
artifacts/operator_delivery/YYYYMMDD-HHMM/operator_notes.md
```

`operator_notes.md`에는 firmware/cfg 파일명, 보드 SN, COM 포트, 시작/종료 시각, PASS/FAIL 판단, 중단 사유를 기록한다.

## 실패 시 중단 조건

아래 조건 중 하나라도 발생하면 현장 배포 승인을 중단하고 원인을 기록한다.

- `check_delivery.ps1`에서 `delivery/firmware.bin` 또는 `delivery/firmware.hex`가 `[WARNING]`으로 표시됨.
- `tools/provision.ps1` 실행 중 ST-Link 미검출, 다중 probe 선택 오류, flash/write 오류가 발생함.
- 잘못된 `-sn` / cfg 조합을 사용했음이 확인됨.
- UART에서 `[UWB][CFG] Load success`가 출력되지 않음.
- UART 역할 출력이 의도한 TAG/ANCHOR와 다름.
- 반복 reset, hard fault, config load failure가 발생함.
- TAG에서 `ok=3/3`가 안정적으로 관측되지 않음.
- TAG에서 반복적인 `overrun=1`이 나타남.
- ANCHOR에서 `bad_pan` 또는 `bad_dst`가 0이 아닌 값으로 증가함.
- 로그를 `artifacts/operator_delivery/` 아래에 보존하지 못함.

## 완료 판정

다음 조건을 모두 만족할 때만 현장 배포 전 확인을 PASS로 기록한다.

- `delivery/firmware.bin`과 `delivery/firmware.hex` freshness가 모두 `[OK]`.
- 4개 보드에 `firmware.bin`과 각 보드 cfg가 오류 없이 적용됨.
- 각 UART 로그가 의도한 역할과 cfg 상태를 보여줌.
- TAG 로그에서 `ok=3/3`와 정상 cycle pacing이 관측됨.
- ANCHOR 로그에서 PAN/address mismatch 또는 config failure가 없음.
- 모든 로그와 운영자 메모가 `artifacts/operator_delivery/` 아래에 저장됨.

## 금지 사항

- 이 runbook 수행 중 firmware source를 수정하지 않는다.
- CMake 또는 build script를 수정하지 않는다.
- `provision.ps1` / `provision.sh`를 수정하지 않는다.
- timing constant를 변경하지 않는다.
- `cfg.bin` binary format을 변경하지 않는다.
- `cfg/presets/*.json`을 SAFE로 승격하지 않는다.
- 새 CANDIDATE preset을 만들지 않는다.
- higher-Hz runtime tuning이나 6 Hz / N=3 베이스라인 재증명을 수행하지 않는다.
