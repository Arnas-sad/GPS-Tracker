/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32f4xx_hal.h"

#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_exti.h"
#include "stm32f4xx_ll_cortex.h"
#include "stm32f4xx_ll_utils.h"
#include "stm32f4xx_ll_pwr.h"
#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_tim.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_gpio.h"

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

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Modem_Power_Off_Pin LL_GPIO_PIN_1
#define Modem_Power_Off_GPIO_Port GPIOE
#define GPS_Fix_LED_Pin LL_GPIO_PIN_4
#define GPS_Fix_LED_GPIO_Port GPIOB
#define Modem_UART_RX_Pin LL_GPIO_PIN_5
#define Modem_UART_RX_GPIO_Port GPIOD
#define Modem_UART_DTR_Pin LL_GPIO_PIN_7
#define Modem_UART_DTR_GPIO_Port GPIOD
#define Modem_ON_Pin LL_GPIO_PIN_15
#define Modem_ON_GPIO_Port GPIOA
#define Stat_LED_Pin LL_GPIO_PIN_5
#define Stat_LED_GPIO_Port GPIOB
#define Modem_UART_TX_Pin LL_GPIO_PIN_6
#define Modem_UART_TX_GPIO_Port GPIOD
#define Debug_TX_Pin LL_GPIO_PIN_6
#define Debug_TX_GPIO_Port GPIOB
#define Modem_UART_CTS_Pin LL_GPIO_PIN_4
#define Modem_UART_CTS_GPIO_Port GPIOD
#define Debug_RX_Pin LL_GPIO_PIN_7
#define Debug_RX_GPIO_Port GPIOB
#define Modem_UART_RTS_Pin LL_GPIO_PIN_3
#define Modem_UART_RTS_GPIO_Port GPIOD
#define GNSS_On_Pin LL_GPIO_PIN_7
#define GNSS_On_GPIO_Port GPIOG
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
