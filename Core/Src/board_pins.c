#include "board_pins.h"

void board_uwb_cs_assert(void)
{
    HAL_GPIO_WritePin(BOARD_UWB_CS_GPIO_Port, BOARD_UWB_CS_Pin, GPIO_PIN_RESET);
}

void board_uwb_cs_deassert(void)
{
    HAL_GPIO_WritePin(BOARD_UWB_CS_GPIO_Port, BOARD_UWB_CS_Pin, GPIO_PIN_SET);
}

void board_uwb_reset_assert(void)
{
    HAL_GPIO_WritePin(BOARD_UWB_RST_GPIO_Port, BOARD_UWB_RST_Pin, GPIO_PIN_RESET);
}

void board_uwb_reset_deassert(void)
{
    HAL_GPIO_WritePin(BOARD_UWB_RST_GPIO_Port, BOARD_UWB_RST_Pin, GPIO_PIN_SET);
}

bool board_uwb_irq_read(void)
{
    return (HAL_GPIO_ReadPin(BOARD_UWB_IRQ_GPIO_Port, BOARD_UWB_IRQ_Pin) == GPIO_PIN_SET);
}
