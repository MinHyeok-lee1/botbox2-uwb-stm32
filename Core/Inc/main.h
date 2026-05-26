/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#ifndef PRODUCT_EXPERIMENTAL_STOP1_SLOW_LISTEN
/* Product-only experiment. Default OFF: release/customer-export builds keep
 * the accepted S2 MCU RUN + DW3000 RX-OFF/RX enable slow-listen fallback. */
#define PRODUCT_EXPERIMENTAL_STOP1_SLOW_LISTEN 0
#endif

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
#if PRODUCT_EXPERIMENTAL_STOP1_SLOW_LISTEN
void product_stop1_slow_listen_lptim_irq(void);
#endif

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define USER_BUTTON_Pin GPIO_PIN_13
#define USER_BUTTON_GPIO_Port GPIOC
#define SW1_Pin GPIO_PIN_2
#define SW1_GPIO_Port GPIOC
#define SW2_Pin GPIO_PIN_3
#define SW2_GPIO_Port GPIOC
#define SW3_Pin GPIO_PIN_0
#define SW3_GPIO_Port GPIOA
#define SW4_Pin GPIO_PIN_1
#define SW4_GPIO_Port GPIOA
#define VCP_TX_Pin GPIO_PIN_2
#define VCP_TX_GPIO_Port GPIOA
#define VCP_RX_Pin GPIO_PIN_3
#define VCP_RX_GPIO_Port GPIOA
#define UWB_SPI1_CS_Pin GPIO_PIN_4
#define UWB_SPI1_CS_GPIO_Port GPIOA
#define UWB_SPI1_SCK_Pin GPIO_PIN_5
#define UWB_SPI1_SCK_GPIO_Port GPIOA
#define UWB_SPI1_MISO_Pin GPIO_PIN_6
#define UWB_SPI1_MISO_GPIO_Port GPIOA
#define UWB_SPI1_MOSI_Pin GPIO_PIN_7
#define UWB_SPI1_MOSI_GPIO_Port GPIOA
#define UWB_IRQ_Pin GPIO_PIN_4
#define UWB_IRQ_GPIO_Port GPIOC
#define UWB_GPIO4_Pin GPIO_PIN_5
#define UWB_GPIO4_GPIO_Port GPIOC
#define UWB_GPIO5_Pin GPIO_PIN_0
#define UWB_GPIO5_GPIO_Port GPIOB
#define UWB_GPIO6_Pin GPIO_PIN_1
#define UWB_GPIO6_GPIO_Port GPIOB
#define UWB_GPIO7_Pin GPIO_PIN_2
#define UWB_GPIO7_GPIO_Port GPIOB
#define LED_LEFT_Pin GPIO_PIN_9
#define LED_LEFT_GPIO_Port GPIOC
#define LED_RIGHT_Pin GPIO_PIN_8
#define LED_RIGHT_GPIO_Port GPIOA
#define LED_BACK_Pin GPIO_PIN_9
#define LED_BACK_GPIO_Port GPIOA
#define LED_SPARE_Pin GPIO_PIN_10
#define LED_SPARE_GPIO_Port GPIOA
#define UWB_RESET_Pin GPIO_PIN_15
#define UWB_RESET_GPIO_Port GPIOA
#define UWB_WAKEUP_Pin GPIO_PIN_10
#define UWB_WAKEUP_GPIO_Port GPIOC
#define UWB_EXTON_Pin GPIO_PIN_11
#define UWB_EXTON_GPIO_Port GPIOC
#define MODE_AUTO_Pin GPIO_PIN_4
#define MODE_AUTO_GPIO_Port GPIOB
#define MODE_MANUAL_Pin GPIO_PIN_5
#define MODE_MANUAL_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
