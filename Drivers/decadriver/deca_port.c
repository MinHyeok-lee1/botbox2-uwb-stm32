#include "deca_device_api.h"
#include "deca_regs.h"
#include "stm32g4xx_hal.h"
#include "gpio.h"
#include <string.h>
#include <stdio.h>

/* ===== External handles ===== */
extern SPI_HandleTypeDef hspi1;

/* ===== GPIO helpers (CubeMX-generated pin macros) ===== */
#define DWM_CS_LOW()      HAL_GPIO_WritePin(UWB_SPI1_CS_GPIO_Port, UWB_SPI1_CS_Pin, GPIO_PIN_RESET)
#define DWM_CS_HIGH()     HAL_GPIO_WritePin(UWB_SPI1_CS_GPIO_Port, UWB_SPI1_CS_Pin, GPIO_PIN_SET)

#define DWM_RST_LOW()     HAL_GPIO_WritePin(UWB_RESET_GPIO_Port, UWB_RESET_Pin, GPIO_PIN_RESET)
#define DWM_RST_HIGH()    HAL_GPIO_WritePin(UWB_RESET_GPIO_Port, UWB_RESET_Pin, GPIO_PIN_SET)

#define DWM_WU_LOW()      HAL_GPIO_WritePin(UWB_WAKEUP_GPIO_Port, UWB_WAKEUP_Pin, GPIO_PIN_RESET)
#define DWM_WU_HIGH()     HAL_GPIO_WritePin(UWB_WAKEUP_GPIO_Port, UWB_WAKEUP_Pin, GPIO_PIN_SET)

#define DWM_EXTON_LOW()   HAL_GPIO_WritePin(UWB_EXTON_GPIO_Port, UWB_EXTON_Pin, GPIO_PIN_RESET)
#define DWM_EXTON_HIGH()  HAL_GPIO_WritePin(UWB_EXTON_GPIO_Port, UWB_EXTON_Pin, GPIO_PIN_SET)

/* ===== Timing ===== */
extern uint32_t micros(void);   // TIM2 기반

void deca_sleep(unsigned int time_ms)
{
    HAL_Delay(time_ms);
}

void deca_usleep(unsigned long time_us)
{
    uint32_t t0 = micros();
    while ((uint32_t)(micros() - t0) < (uint32_t)time_us) {
        __NOP();
    }
}

/* ===== Mutex ===== */
__attribute__((weak))
decaIrqStatus_t decamutexon(void)
{
    decaIrqStatus_t s = __get_PRIMASK();
    __disable_irq();
    return s;
}

__attribute__((weak))
void decamutexoff(decaIrqStatus_t s)
{
    if ((s & 0x1u) == 0u) {
        __enable_irq();
    }
}

/* ===== SPI glue ===== */
int writetospi(uint16_t headerLength, const uint8_t* headerBuffer, uint16_t bodyLength, const uint8_t* bodyBuffer) {
    DWM_CS_LOW();
    HAL_SPI_Transmit(&hspi1, (uint8_t*)headerBuffer, headerLength, HAL_MAX_DELAY);
    if (bodyLength && bodyBuffer) {
        HAL_SPI_Transmit(&hspi1, (uint8_t*)bodyBuffer, (uint16_t)bodyLength, HAL_MAX_DELAY);
    }
    DWM_CS_HIGH();
    return 0;
}

int readfromspi(uint16_t headerLength, uint8_t* headerBuffer, uint16_t readLength, uint8_t* readBuffer) {
    DWM_CS_LOW();
    HAL_SPI_Transmit(&hspi1, (uint8_t*)headerBuffer, headerLength, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, readBuffer, (uint16_t)readLength, HAL_MAX_DELAY);
    DWM_CS_HIGH();
    return 0;
}

/* CRC variant (optional, keep signature) */
int writetospiwithcrc(uint16_t headerLength,
                      const uint8_t* headerBuffer,
                      uint16_t bodyLength,
                      const uint8_t* bodyBuffer,
                      uint8_t crc8)
{
    (void)crc8;
    return writetospi(headerLength, headerBuffer, bodyLength, bodyBuffer);
}

/* ===== SPI speed control ===== */
void port_set_dw_ic_spi_fastrate(void)
{
    /* CubeMX 설정 기준으로 이미 충분히 빠르면 비워둬도 됨 */
}

void port_set_dw_ic_spi_slowrate(void)
{
    /* 필요 시 prescaler 변경 */
}

/* ===== IRQ glue ===== */
typedef void (*dwic_isr_t)(void);

void port_set_dwic_isr(dwic_isr_t isr)
{
    (void)isr;
    /* 현재는 IRQ 미연결 상태 */
}

/* ===== Bring-up helper ===== */
void deca_port_init(void)
{
    DWM_CS_HIGH();
    DWM_WU_HIGH();
    DWM_RST_HIGH();
}