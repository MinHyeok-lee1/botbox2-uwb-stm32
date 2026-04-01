#include "stm32g4xx_hal.h"

extern TIM_HandleTypeDef htim2;

void platform_time_init(void)
{
    HAL_TIM_Base_Start(&htim2);
}

uint32_t micros(void)
{
    return __HAL_TIM_GET_COUNTER(&htim2);
}

void micros_delay(uint32_t us)
{
    uint32_t t0 = micros();
    while ((uint32_t)(micros() - t0) < us) {
        __NOP();
    }
}
