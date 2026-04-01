#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "main.h"
#include <stdbool.h>

/* --- UWB GPIO Pin Mapping (Hardware-truth Labels) --- */
#define BOARD_UWB_CS_Pin        UWB_SPI1_CS_Pin
#define BOARD_UWB_CS_GPIO_Port  UWB_SPI1_CS_GPIO_Port

#define BOARD_UWB_RST_Pin       UWB_RESET_Pin
#define BOARD_UWB_RST_GPIO_Port UWB_RESET_GPIO_Port

#define BOARD_UWB_IRQ_Pin       UWB_IRQ_Pin
#define BOARD_UWB_IRQ_GPIO_Port UWB_IRQ_GPIO_Port

/* --- BAL Logic Prototypes --- */

void board_uwb_cs_assert(void);
void board_uwb_cs_deassert(void);
void board_uwb_reset_assert(void);
void board_uwb_reset_deassert(void);
bool board_uwb_irq_read(void);

#endif /* BOARD_PINS_H */
