#include "can_controller.h"
#include "fdcan.h"
#include "main.h"

/**
 * @brief  정수형 데이터 길이를 FDCAN DLC 매크로로 변환하는 내부 헬퍼 함수
 * @param  length 전송할 데이터 바이트 수 (0~8)
 * @retval FDCAN DLC 매크로
 */
static uint32_t get_fdcan_dlc(uint8_t length) {
  switch (length) {
  case 0:
    return FDCAN_DLC_BYTES_0;
  case 1:
    return FDCAN_DLC_BYTES_1;
  case 2:
    return FDCAN_DLC_BYTES_2;
  case 3:
    return FDCAN_DLC_BYTES_3;
  case 4:
    return FDCAN_DLC_BYTES_4;
  case 5:
    return FDCAN_DLC_BYTES_5;
  case 6:
    return FDCAN_DLC_BYTES_6;
  case 7:
    return FDCAN_DLC_BYTES_7;
  case 8:
    return FDCAN_DLC_BYTES_8;
  // CAN FD를 사용하시어 8바이트 이상을 전송하신다면 아래 주석을 해제하여
  // 사용하세요.
  /*
  case 12: return FDCAN_DLC_BYTES_12;
  case 16: return FDCAN_DLC_BYTES_16;
  case 20: return FDCAN_DLC_BYTES_20;
  case 24: return FDCAN_DLC_BYTES_24;
  case 32: return FDCAN_DLC_BYTES_32;
  case 48: return FDCAN_DLC_BYTES_48;
  case 64: return FDCAN_DLC_BYTES_64;
  */
  default:
    return FDCAN_DLC_BYTES_8; // 기본값
  }
}

/**
 * @brief  FDCAN 동작 시작
 * @param  hfdcan FDCAN 핸들 포인터
 */
void can_start() {

  if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
    // Start 실패 시 에러 처리 로직 (예: Error_Handler();)
  }
}

/**
 * @brief  FDCAN 메시지 송신 (Tx)
 * @param  hfdcan FDCAN 핸들 포인터
 * @param  id 송신할 CAN ID
 * @param  data 전송할 데이터 배열의 포인터
 * @param  length 전송할 데이터 길이 (Classic CAN 기준 최대 8)
 * @retval HAL Status
 */
HAL_StatusTypeDef can_tx_msg(uint32_t id, uint8_t *data, uint8_t length) {
  FDCAN_TxHeaderTypeDef TxHeader;

  /* Tx Header 설정 */
  TxHeader.Identifier = id;
  TxHeader.IdType = FDCAN_STANDARD_ID;
  // 확장 ID(29-bit) 사용 시 FDCAN_EXTENDED_ID 로 변경
  TxHeader.TxFrameType = FDCAN_DATA_FRAME;     // 데이터 프레임 전송
  TxHeader.DataLength = get_fdcan_dlc(length); // 헬퍼 함수를 통한 DLC 설정

  /* FDCAN 특화 설정 (기본 Classic CAN 모드 기준) */
  TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
  // FD 통신 시 Bit Rate Switch 사용하면 ON으로 변경
  TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
  // FD 통신 시 FDCAN_FD_CAN 으로 변경
  TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  TxHeader.MessageMarker = 0;

  /* 메시지를 Tx FIFO 또는 Queue에 적재 */
  return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, data);
}