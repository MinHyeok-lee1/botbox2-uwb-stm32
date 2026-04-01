#include "deca_device_api.h"
#include "deca_regs.h"
#include "board_pins.h"
#include "platform_time.h"
#include <string.h>
#include <stdio.h>

/* ===== External handles ===== */
extern SPI_HandleTypeDef hspi1;

/* ===== Timing ===== */
extern uint32_t micros(void);   // TIM2 기반 (STEP1에서 검증)

void deca_sleep(unsigned int time_ms)
{
    HAL_Delay(time_ms);
}

void deca_usleep(unsigned long time_us)
{
    micros_delay((uint32_t)time_us);
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
    if ((s & 0x1u) == 0) {
        __enable_irq();
    }
}

/* ===== SPI glue ===== */
int writetospi(uint16_t headerLength, const uint8_t* headerBuffer, uint16_t bodyLength, const uint8_t* bodyBuffer) {
    board_uwb_cs_assert();
    HAL_SPI_Transmit(&hspi1, (uint8_t*)headerBuffer, headerLength, HAL_MAX_DELAY);
    if (bodyLength && bodyBuffer) {
        HAL_SPI_Transmit(&hspi1, (uint8_t*)bodyBuffer, (uint16_t)bodyLength, HAL_MAX_DELAY);
    }
    board_uwb_cs_deassert();
    return 0;
}

int readfromspi(uint16_t headerLength, uint8_t* headerBuffer, uint16_t readLength, uint8_t* readBuffer) {
    board_uwb_cs_assert();
    HAL_SPI_Transmit(&hspi1, (uint8_t*)headerBuffer, headerLength, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, readBuffer, (uint16_t)readLength, HAL_MAX_DELAY);
    board_uwb_cs_deassert();
    return 0;
}

/* CRC variant (optional, keep signature) */
int writetospiwithcrc(uint16_t headerLength, const uint8_t* headerBuffer, uint16_t bodyLength, const uint8_t* bodyBuffer, uint8_t crc8) {
    (void)crc8;
    return writetospi(headerLength, headerBuffer, bodyLength, bodyBuffer);
}

/* ===== SPI speed control ===== */
void port_set_dw_ic_spi_fastrate(void)
{
    // CubeMX 설정 기준으로 이미 충분히 빠르면 비워둬도 됨
}

void port_set_dw_ic_spi_slowrate(void)
{
    // 필요 시 prescaler 변경
}

/* ===== IRQ glue (STEP3-2에서 연결) ===== */
typedef void (*dwic_isr_t)(void);
void port_set_dwic_isr(dwic_isr_t isr)
{
    (void)isr;
    // STEP3-1에서는 비워둔다
}

/* ===== Bring-up helper ===== */
void deca_port_init(void)
{
    board_uwb_cs_deassert();
    board_uwb_reset_deassert();
    // Wakeup fallback (manual mapping for now if not in board_pins)
    HAL_GPIO_WritePin(UWB_WAKEUP_GPIO_Port, UWB_WAKEUP_Pin, GPIO_PIN_SET); 
}