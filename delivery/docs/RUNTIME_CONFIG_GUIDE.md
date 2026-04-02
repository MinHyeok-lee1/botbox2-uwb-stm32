# STM32-DW3000 UWB Runtime Configuration Guide

## 1. 운영 개념 (Operational Overview)

### 제품 운영 철학
본 UWB 솔루션은 **"고정형 펌웨어 + 교체형 설정"** 원칙으로 설계되어, 현장 운영의 복잡성을 최소화합니다.

- **Firmware Asset (`firmware.bin`)**: UWB 통신 엔진, 필터, 링커 등 핵심 논리를 포함하는 단일 통합 실행 파일입니다. 현장에 한 번 배포된 후에는 잦은 변경이 발생하지 않습니다.
- **Config Asset (`cfg.bin`)**: 디바이스의 역할(TAG/ANCHOR), 물리 주소(PAN ID, Short Addr), 대상 앵커 목록 등 현장 맞춤형 데이터를 담고 있는 40바이트의 설정 파일입니다.

**현장 운영의 핵심**: 
단순히 장비의 역할을 바꾸거나 주소를 재할당하려면 펌웨어 전체를 재설치하거나 다시 빌드할 필요가 없습니다. 용도에 맞는 `cfg.bin` 파일 하나만 교체(프로비저닝)하면 즉시 변경 사항이 적용됩니다.

---

## 2. 설정 필드 이해 (Configuration Fields)

미리 배포된 설정 파일(`cfg.bin`)들은 내부적으로 디바이스의 동작을 제어하는 파라미터들을 담고 있습니다.

### 역할 및 주소 (Role & Network)
- **Role (역할)**: `TAG`(이동 측위 기기) 또는 `ANCHOR`(고정 기준 기기)
- **PAN ID**: 동일 공간 내 UWB 장비 그룹을 묶는 고유번호 (예: `0x2001`)
- **Short Address**: 해당 그룹 내 각 디바이스의 고유 식별자 (PAN ID가 같은 기기끼리는 Short Address가 겹치면 안 됩니다.)
- **Anchors**: 기기가 통신할 대상 앵커들의 주소 목록 (최대 8개)

### 통신 및 필터 (Ranging & Filter)
- **Ranging**: 측위 프로토콜 (현재 버전 `SS` TWR 모드 고정 권장)
- **Filter**: 노이즈 억제 및 필터링 강도 (`WEAK`, `NORMAL`, `STRONG`)
- **Comm Mode**: 통신 다중화 모드 (현재 버전 `ANYCAST` 고정 권장)

> **표준 배포 프리셋 정규 목록 (Standard Presets)**: 현장의 편의성을 극대화하기 위해 `configs/` 경로에는 사전 검증이 완료된 표준 설정 파일들이 동봉되어 있습니다. 오퍼레이터는 아래 파일명 중 하나를 `-cfg` 인자로 선택하여 1초 만에 디바이스 토폴로지를 구성할 수 있습니다.
> - **Golden Presets** (제품 초기화용 기본 팩): 
>   - `tag_default.bin`, `anchor_default.bin`
> - **Operational Presets** (현장 표준 1-TAG, 3-ANCHOR 세트):
>   - `tag_pan2001_addr1001.bin`
>   - `anchor_pan2001_addr2001.bin`
>   - `anchor_pan2001_addr2002.bin`
>   - `anchor_pan2001_addr2003.bin`
> 
> *참고: 위 표준 세트 외의 특수 목적 변형이 필요할 때에 한해, 엔지니어링 팀에서 제공되는 `gen_cfg.py` 도구를 통해 운영자가 파생 설정 파일을 직접 생성(`Engineering Workflow`)할 수 있습니다.*

---

## 3. 현장 운영 가이드 (Provisioning Procedures)

배포 패키지(`delivery/`) 안에 포함된 `tools\provision.ps1` (Windows용) 스크립트를 사용하여 디바이스를 빠르고 안전하게 설정합니다.

### 시나리오 A: 신규 디바이스 완전 배포 (Full Provisioning)
처음 설치하는 장비에는 펌웨어와 기본 설정을 함께 주입해야 합니다.
```powershell
.\tools\provision.ps1 -fw firmware.bin -cfg configs\tag_default.bin
```

### 시나리오 B: 작동 중인 디바이스 역할 전환 (Config Update Only)
디바이스를 TAG에서 ANCHOR로 변경할 때 펌웨어는 덮어쓸 필요가 없습니다. 설정 파일만 안전하게 교체합니다.
```powershell
.\tools\provision.ps1 -cfg configs\anchor_default.bin
```

### 시나리오 C: 펌웨어 엔진 업데이트 (Firmware Update Only)
기존에 설정해 둔 역할이나 네트워크 주소(Config)를 그대로 유지하면서, 코어 펌웨어 모듈만 최신화할 때 사용합니다.
```powershell
.\tools\provision.ps1 -fw firmware.bin
```

### (참고) Python 도구를 활용한 파생 설정 생성 (Engineering Workflow)
파이썬 구동 환경이 준비된 엔지니어에 한하여, 현장에서 즉각적인 주소 변형이 필요할 때 사용합니다. 생성된 파일은 시나리오 B와 동일하게 주입합니다.
```bash
# 예: ANCHOR 역할, PAN 0x3005, 단말주소 0x2009 설정 파일 생성
python tools\gen_cfg.py --role ANCHOR --pan 0x3005 --addr 0x2009 -o delivery\configs\my_node.bin --verify

# 생성된 설정 주입
.\tools\provision.ps1 -cfg delivery\configs\my_node.bin
```

---

## 4. 트러블슈팅 (Troubleshooting)

### 에러: `Multiple probes found. Please specify -sn <Serial>.`
- **원인**: PC에 STM32 디바이스 2대 이상이 연결되어 있어 오작동 방지를 위해 쓰기가 차단되었습니다.
- **해결**: 터미널에 동시에 출력된 시리얼 번호 목록 중 대상 기기의 시리얼(예: `004D00...`)을 복사한 뒤, 명령어의 끝에 `-sn [복사한 시리얼]` 옵션을 추가하여 명시적으로 재실행하십시오.
- **예시**: `.\tools\provision.ps1 -cfg configs\anchor_default.bin -sn 004D003D3234510F37333934`

### 에러: `No ST-Link probe found.`
- **원인**: 디바이스 연결이 불안정하거나 케이블 단선, 혹은 프로브가 인식되지 않았습니다.
- **해결**: 대상 보드 케이블 재연결 직후 장치 관리자 인식을 확인하고, 다시 명령을 수행하십시오.

### 에러: `Config size (... bytes) != expected 40 bytes.` (또는 Firmware size exceeds limit)
- **원인**: 유효하지 않은 크기의 바이너리나 손상된 파일을 주입하려 시도했습니다. 시스템 보호를 위해 프로비저닝 엔진 단계에서 원천 차단된 상태입니다.
- **해결**: 원본 `delivery` 패키지의 무결성을 확인하고, 올바른 폴더 경로의 `.bin`을 지정했는지 재확인하세요.

### 에러: `STM32CubeProgrammer not found or not in PATH.`
- **해결**: 프로비저닝 스크립트 실행에 필요한 필수 코어가 시스템에 설치되어 있지 않습니다. STMicroelectronics 공식 홈페이지에서 `STM32CubeProgrammer`를 기본 경로에 설치한 후 재설치하여 주십시오.

---

### 비상 수동 복구 경로 (Emergency GUI Fallback)
스크립트(`provision.ps1/sh`)를 실행할 수 없는 극한의 비상 상황 시에만 STM32CubeProgrammer GUI 화면을 구동해 수동으로 주입합니다. **이 경로는 권장되지 않습니다.**
1. `firmware.bin` 로드 → 다운로드 주소 **0x08000000** 지정 후 Write
2. `cfg.bin` 로드 → 다운로드 주소 **0x0807F800** 지정 후 Write 
*(경고: 주소 지정 오류 시 전체 코어 손상이 발생할 수 있으므로, 반드시 표준 스크립트를 사용하십시오.)*
