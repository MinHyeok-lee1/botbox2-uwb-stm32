#ifndef CAN_CONTROLLER_H
#define CAN_CONTROLLER_H

#include "main.h"

/* FDCAN 초기화 (Start 및 필요한 Filter 설정) */
void can_start();

/* FDCAN 메시지 송신 함수 */
HAL_StatusTypeDef can_tx_msg(uint32_t id, uint8_t *data, uint8_t length);

#endif /* CAN_CONTROLLER_H */