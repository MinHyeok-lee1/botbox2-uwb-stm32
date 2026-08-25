# 제품 프로비저닝 가이드 (PRODUCT PROVISIONING GUIDE)

본 문서는 보드별 역할 할당 및 펌웨어 배포를 위한 프로비저닝(Provisioning) 가이드를 제공합니다.

## 1. 배포 모델 (Deployment Model)
본 프로젝트는 **고정 펌웨어(Golden Firmware) + 교체 가능한 설정(cfg)** 모델을 따릅니다.
- 운영 목적에 맞는 explicit Customer firmware asset을 선택합니다. 기본 CH9 운영은 `firmware_product_ch9.bin`, CH5 운영은 `firmware_product_ch5.bin`, RXDIAG 검증은 `firmware_rxdiag_ch5.bin` 또는 `firmware_rxdiag_ch9.bin`을 사용합니다.
- 각 보드의 역할(TAG/ANCHOR), ID, 네트워크 설정은 `cfg.bin` 파일에 의해 결정됩니다.
- legacy `firmware.bin`은 현재 Customer release asset이 아닙니다.
- 02 Product의 `firmware_product_ch9_tdma_optimal.bin`은 Product-only opt-in artifact이며 Customer canonical delivery에 포함하지 않습니다.

## 2. 프로비저닝 도구 (Provisioning Tools)
`tools/provision.ps1` 스크립트를 사용하여 펌웨어와 설정을 한 번에 기록할 수 있습니다.

### 사용 방법 (Usage)
```powershell
powershell -ExecutionPolicy Bypass -File .\tools\provision.ps1 -sn <ST_LINK_SN> -fw .\delivery\firmware_product_ch9.bin -cfg .\delivery\configs\<cfg>.bin
```

- **펌웨어 (Firmware)**: 기본 CH9는 `delivery/firmware_product_ch9.bin`; 필요 시 CH5 또는 RXDIAG explicit asset을 선택
- **설정 파일 (Configs)**: `delivery/configs/*.bin` (보드별 역할에 맞는 파일 선택)

## 3. 설정 구성 요소 (Configuration Components - cfg)
`cfg.bin`은 다음 항목을 제어합니다.
- 동작 역할 (TAG 또는 ANCHOR)
- PAN ID 및 단축 주소 (Short Address)
- 앵커 리스트 및 슬롯 정보
- 레인징 모드 (SS-TWR 또는 DS-TWR)

## 4. 주의 사항 (Warning)
- **보드 적합성**: EVB 전용 설정이나 테스트용 cfg를 제품 보드에 무분별하게 사용하지 마십시오. 반드시 `delivery/configs/` 내의 제품용 설정 파일을 사용해야 합니다.
- **주소 충돌**: 동일한 네트워크(PAN ID) 내에서 단축 주소가 충돌하지 않도록 주의하십시오.
- **하드웨어 제약**: 제품 보드 고유의 클럭 및 핀맵 설정은 펌웨어에 고정되어 있으므로, 설정 파일 변경만으로 하드웨어 특성을 바꿀 수 없습니다.
<!-- Keywords: cmake -->
