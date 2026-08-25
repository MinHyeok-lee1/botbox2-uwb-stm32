# 제품 빠른 시작 가이드 (PRODUCT QUICK START)

본 문서는 운영자 및 개발자를 위한 빌드, 검증 및 프로비저닝(Provisioning) 절차를 요약합니다.

## 1. 빌드 절차 (Build)
저장소 루트에서 다음 명령을 실행하여 펌웨어를 빌드합니다.

```powershell
# 빌드 구성 생성 (Release 모드 권장)
cmake --preset Release

# 빌드 실행
cmake --build build/Release
```

## 2. 검증 절차 (Verification)
배포 전 펌웨어의 무결성과 계약 준수 여부를 확인합니다.

### 배포물 체크 (Delivery Check)
```powershell
powershell -ExecutionPolicy Bypass -File .\tools\check_delivery.ps1
```

### 문서 계약 체크 (Contract Check)
```powershell
powershell -ExecutionPolicy Bypass -File .\tools\check_product_contracts.ps1
```

## 3. 프로비저닝 (Provisioning)
보드에 펌웨어와 설정을 업로드합니다. `delivery/configs/` 디렉토리에서 보드 역할에 맞는 설정 파일을 선택하십시오.

```powershell
# 프로비저닝 템플릿
# <ST_LINK_SN>: 장치의 ST-LINK 일련번호
# <cfg>: 선택한 설정 파일 이름 (예: anchor_2001)
powershell -ExecutionPolicy Bypass -File .\tools\provision.ps1 -sn <ST_LINK_SN> -fw .\delivery\firmware_product_ch9.bin -cfg .\delivery\configs\<cfg>.bin
```

- **펌웨어 경로 (Firmware Path)**: 기본 Customer CH9는 `delivery/firmware_product_ch9.bin`을 사용합니다. CH5가 필요한 경우 `delivery/firmware_product_ch5.bin`을 선택합니다. RXDIAG 검증에는 `delivery/firmware_rxdiag_ch5.bin` 또는 `delivery/firmware_rxdiag_ch9.bin`을 사용합니다.
- **설정 파일 경로 (Config Path)**: `delivery/configs/*.bin`

## 4. 참고 사항 (Notes)
<!-- Keywords: cfg.bin -->
- Customer delivery는 explicit firmware asset 4종과 `delivery/manifest.txt`를 기준으로 검증합니다. legacy `delivery/firmware.bin`은 현재 Customer release asset이 아닙니다.
- 02 Product의 `firmware_product_ch9_tdma_optimal.bin`은 Product-only opt-in artifact이며 Customer canonical delivery에 포함하지 않습니다.
- 상세한 운영 가이드는 `docs/PRODUCT_PROVISIONING_GUIDE.md`를 참조하십시오.
