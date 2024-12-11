/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdio.h>
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_utils.h"
#include "stm32f4xx_ll_pwr.h"
#include "stm32f4xx_ll_bus.h"
#include "cmsis_os2.h"
#include "tim_driver.h"
#include "gpio_driver.h"
#include "uart_api.h"
#include "string_util.h"
#include "debug_api.h"
#include "cli_app.h"
#include "cli_commands.h"
#include "led_api.h"
#include "led_app.h"
#include "modem_api.h"
#include "tcp_api.h"
#include "tcp_app.h"
/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/
//CREATE_MODULE_TAG(MAIN_C);
//static const osThreadAttr_t thread2_attributes = {
//	.name = "Task_start",
//	.stack_size = 256,
//	.priority = 21
//};
/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/
CREATE_MODULE_TAG(MAIN);
/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/
static void SystemClock_Config (void);
void Thread_Task2(void *arg);
//void Thread_Task3(void *arg);
/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/
static void SystemClock_Config (void) {
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_3);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_3) {
    }
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
    LL_RCC_HSI_SetCalibTrimming(16);
    LL_RCC_HSI_Enable();
    while (LL_RCC_HSI_IsReady() != 1) {
    }
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_8, 100, LL_RCC_PLLP_DIV_2);
    LL_RCC_PLL_Enable();
    while (LL_RCC_PLL_IsReady() != 1) {
    }
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
    }
    LL_SetSystemCoreClock(100000000);
    if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
        __disable_irq();

        while (1) {

        }
    }
    LL_RCC_SetTIMPrescaler(LL_RCC_TIM_PRESCALER_TWICE);
}

//void Thread_Task2(void *arg) {
//  while (1) {
//      GPIO_Driver_Toggle(eGPIODriver_LedGpsFixPin);
//      GPIO_Driver_Toggle(eGPIODriver_StatusLedPin);
//      osDelay(250);
//  }
//}

//void Thread_Task2(void *arg) {
//
//   while (1) {
//       sString_t msg = {0};
//       if (UART_API_GetMessage(eUartApiDevice_Debug, &msg, osWaitForever) == true) {
//           UART_API_SendMessage(eUartApiDevice_Debug, msg);
//       }
//
//       osThreadYield();
//   }
//}

// void Thread_Task2(void *arg) {

//     osKernel
//    while (1) {
//        //DEBUG_WARN("This is a warning message: Apples count = %d \r\n", apples_cnt);
//        osThreadYield();
//    }
// }

//void Thread_Task3(void *arg) {
////        sString_t delimiter = (sString_t)DEFINE_STRING("\r\n");
////        UART_API_Init(eUartApiDevice_Debug, 115200, delimiter);
////        Debug_API_Init();
//    CLI_APP_Init();
//////        DEBUG_INFO("UART Command Line interface has been initialized correctly!! \r\n");
//////    }
////    eUartApiDevice_t uart = eUartApiDevice_Debug;
////    sString_t msg;
////    msg.str = "SUCCESS!";
////    msg.size = 10;
//
//    while (1) {
//        //osThreadYield();
//    }
//}
/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/
int main (void) {

    HAL_Init();

    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    NVIC_SetPriority(PendSV_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));

    SystemClock_Config();
//    GPIO_Driver_Write(eGPIODriver_ModemOnPin, eGPIO_PinState_High);

    if (GPIO_Driver_Init() == false) {
        DEBUG_INFO("Not all pins initialized!");
    }
    //MX_USART1_UART_Init();
    // MX_USART2_UART_Init();

    TIM13_Init();
    // LED_API_LedInit();
   //sString_t delimiter = (sString_t)DEFINE_STRING("\r\n");
    // UART_API_Init(eUartApiDevice_Debug, 115200, delimiter);

    osKernelInitialize();
    if (Debug_API_Init() == false) {
        DEBUG_INFO("DEBUG API INIT failed!\r\n");
    }

    if (LED_API_LedInit() == false) {
        DEBUG_INFO("LED API INIT failed!\r\n");
    }

    if (CLI_APP_Init() == false) {
        DEBUG_INFO("CLI APP INIT failed!\r\n");
    }

    if (LED_APP_Init() == false) {
        DEBUG_INFO("LED APP INIT failed!\r\n");
    }

    if (Modem_API_Init() == false) {
        DEBUG_INFO("MODEM API INIT failed!\r\n");
    }

    if (TCP_APP_Init() == false) {
        DEBUG_INFO("TCP APP INIT failed!\r\n");
    }
    //CLI_APP_Init();
    //LED_API_LedInit();
//    osThreadNew(Thread_Task2, NULL, &thread2_attributes);
    osKernelStart();

//    if (Debug_API_Init() == false) {
//        return -1;
//    }

    while (1) {

    }
}

void HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        HAL_IncTick();
    }
}
