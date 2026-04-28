# IOC-Shell Post-Patch Bring-up Status Audit

## 1. 목적
본 문서는 stm32-dw3000-g474re-ioc-shell 프로젝트의 현재 상태를 파악하기 위한 감사(Audit) 결과를 기록한다. README.md, docs/task.md, 그리고 실제 코드(main.c) 간의 상태 불일치를 확인하고, 현재 진행 단계를 정확히 식별하여 다음 하드웨어 Bring-up 단계로 넘어가기 위한 기준선을 제공한다.

## 2. README vs task.md vs main.c 상태 차이
세 가지 주요 진실의 원천(Source of Truth)을 대조한 결과, 심각한 상태 불일치(Outdated info)가 확인되었다.

*   **README.md (Outdated)**
    *   상태: "Pre-IOC Migration snapshot"
    *   주장: "no UWB application code has been ported yet" 및 다음 작업이 "Start of Real Migration"이라고 기술됨.
*   **docs/task.md (Current)**
    *   상태: "Patched / Ready for bring-up"
    *   주장: Pin mapping, main dispatch patch, printf, early UWB bring-up debug prints 등이 완료되었다고 기록됨.
*   **Core/Src/main.c (Actual Code)**
    *   상태: UWB 어플리케이션 코드가 대거 이식되어 있음.
    *   관찰:
        *   `uwb_lib_init()`, `dw3000_reset()` 등의 초기화 경로 존재
        *   `UWB_RESET_GPIO_Port`, `UWB_SPI_CS_GPIO_Port` 등 Pin mapping 매크로 갱신됨
        *   `uwb_cfg_t` 기반 설정 로직 반영됨
        *   `tx_poll_msg`, `poll_one_aid` 등 SS-TWR 관련 로직 포함됨
        *   `printf("[STEP3] Device id: ...")`와 같은 Early debug print 경로가 삽입됨

## 3. 현재 confirmed-by-document 상태
문서와 코드를 교차 검증한 결과, 본 프로젝트의 현재 상태는 다음과 같이 확정된다.
*   **NOT "pre-port"**: 기초 뼈대만 있는 셸 상태가 아니다.
*   **YES "patched / ready for bring-up"**: UWB 핵심 로직이 이미 main.c에 패치되었으며, 통신 및 디버깅을 위한 UART(printf) 설정과 초기화 검증 코드가 준비되어 있다.
*   실제 하드웨어 구동 준비가 완료된 **Bring-up 직전 상태**이다.

## 4. 현재 unknown / not yet evidenced 상태
*   **Hardware-confirmed 여부**: UNKNOWN
*   코드 상에 SS-TWR 로직이 포함되어 있으나, 이를 실제로 타겟 보드에서 빌드, 플래싱, 실행하여 정상 동작을 확인했다는 **로그나 테스트 결과 문서는 존재하지 않는다**. 따라서 "이미 hardware에서 SS-TWR 동작 확인 완료" 상태로 볼 수 없으며, 단순히 "bring-up 준비 완료" 상태로 간주한다.

## 5. 보존해야 할 Target IOC Pin Truth
README.md와 .ioc 설정에 기반한 하드웨어팀의 Seed IOC Pinout은 계속 보존되어야 한다. (기존 Root main.c와 불일치하던 부분)
*   `PA4` = `UWB_SPI1_CS`
*   `PA15` = `UWB_RESET`
*   `PA8` = `LED_RIGHT`

## 6. README.md 수정 제안 방향
현재 단계에서 README.md를 전면 수정하지 않으나, 차후 다음 사항들이 반영되어야 한다.
*   "Pre-IOC Migration snapshot" 문구 제거 및 "Post-Migration Bring-up" 단계로 갱신
*   "no UWB application code has been ported yet" 항목을 삭제하고, "UWB core logic and SS-TWR path ported"로 갱신
*   Next Task를 "Start of Real Migration"에서 "Hardware Bring-up and Testing"으로 변경

## 7. 다음 실행 단계
본 감사 이후 수행해야 할 액션 플랜은 다음과 같다.
1.  **Clean build**: 변경된 CMake 및 소스 코드로 전체 빌드 수행
2.  **Flash**: 타겟 보드(STM32G474RE)에 펌웨어 업로드
3.  **UART boot log 확인**: 터미널을 통해 초기 부팅 로그 및 `printf` 출력 확인
4.  **DW3000 device id 확인**: `[STEP3] Device id: 0x...` 로그 정상 출력 여부 확인 (SPI 통신 검증)
5.  **Cfg load / role dispatch 확인**: 설정 로드 및 TAG/ANCHOR 역할 분배 동작 확인
6.  **SS-TWR TAG/ANCHOR smoke 확인**: 단일 TAG - 단일 ANCHOR 간 기본 거리 측정 프레임 교환(smoke test) 확인
