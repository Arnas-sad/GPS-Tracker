/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f413xx.h"
#include "gpio_driver.h"
/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/
 typedef struct sGpioDesc_t {
    GPIO_TypeDef *port;
    uint32_t pin;
    uint32_t mode;
    uint32_t speed;
    uint32_t output;
    uint32_t pull;
    uint32_t clock;
    uint32_t alternate;
 } sGpioDesc_t;
/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/
const static sGpioDesc_t g_static_gpio_lut[eGPIODriver_Last] = {
    [eGPIODriver_ModemPowerOffPin] = {.port = GPIOE,                      .pin = LL_GPIO_PIN_1,
                                      .mode = LL_GPIO_MODE_OUTPUT,        .speed = LL_GPIO_SPEED_FREQ_LOW,
                                      .output = LL_GPIO_OUTPUT_PUSHPULL,  .pull = LL_GPIO_PULL_NO, 
                                      .clock = LL_AHB1_GRP1_PERIPH_GPIOE, .alternate = LL_GPIO_AF_0},
                                        
    [eGPIODriver_LedGpsFixPin]     = {.port = GPIOB,                      .pin = LL_GPIO_PIN_4, 
                                      .mode = LL_GPIO_MODE_OUTPUT,        .speed = LL_GPIO_SPEED_FREQ_LOW,
                                      .output = LL_GPIO_OUTPUT_PUSHPULL,  .pull = LL_GPIO_PULL_NO, 
                                      .clock = LL_AHB1_GRP1_PERIPH_GPIOB, .alternate = LL_GPIO_AF_0},
                                        
    [eGPIODriver_ModemUartRXPin]   = {.port = GPIOD,                      .pin = LL_GPIO_PIN_5, 
                                      .mode = LL_GPIO_MODE_ALTERNATE,     .speed = LL_GPIO_SPEED_FREQ_VERY_HIGH,
                                      .output = LL_GPIO_OUTPUT_PUSHPULL,  .pull = LL_GPIO_PULL_NO, 
                                      .clock = LL_AHB1_GRP1_PERIPH_GPIOD, .alternate = LL_GPIO_AF_7},
                                        
    [eGPIODriver_ModemUartDtrPin]  = {.port = GPIOD,                      .pin = LL_GPIO_PIN_7, 
                                      .mode = LL_GPIO_MODE_OUTPUT,        .speed = LL_GPIO_SPEED_FREQ_LOW,
                                      .output = LL_GPIO_OUTPUT_PUSHPULL,  .pull = LL_GPIO_PULL_NO, 
                                      .clock = LL_AHB1_GRP1_PERIPH_GPIOD, .alternate = LL_GPIO_AF_0},
                                            
    [eGPIODriver_ModemOnPin]       = {.port = GPIOA,                      .pin = LL_GPIO_PIN_15, 
                                      .mode = LL_GPIO_MODE_OUTPUT,        .speed = LL_GPIO_SPEED_FREQ_LOW,
                                      .output = LL_GPIO_OUTPUT_PUSHPULL,  .pull = LL_GPIO_PULL_NO, 
                                      .clock = LL_AHB1_GRP1_PERIPH_GPIOA, .alternate = LL_GPIO_AF_0},
                                        
    [eGPIODriver_StatusLedPin]     = {.port = GPIOB,                      .pin = LL_GPIO_PIN_5, 
                                      .mode = LL_GPIO_MODE_OUTPUT,        .speed = LL_GPIO_SPEED_FREQ_LOW,
                                      .output = LL_GPIO_OUTPUT_PUSHPULL,  .pull = LL_GPIO_PULL_NO, 
                                      .clock = LL_AHB1_GRP1_PERIPH_GPIOB, .alternate = LL_GPIO_AF_0},
                                        
    [eGPIODriver_ModemUartTxPin]   = {.port = GPIOD,                      .pin = LL_GPIO_PIN_6, 
                                      .mode = LL_GPIO_MODE_ALTERNATE,     .speed = LL_GPIO_SPEED_FREQ_VERY_HIGH,
                                      .output = LL_GPIO_OUTPUT_PUSHPULL,  .pull = LL_GPIO_PULL_NO, 
                                      .clock = LL_AHB1_GRP1_PERIPH_GPIOD, .alternate = LL_GPIO_AF_7},
                                        
    [eGPIODriver_DebugTxPin]       = {.port = GPIOB,                      .pin = LL_GPIO_PIN_6, 
                                      .mode = LL_GPIO_MODE_ALTERNATE,     .speed = LL_GPIO_SPEED_FREQ_VERY_HIGH,
                                      .output = LL_GPIO_OUTPUT_PUSHPULL,  .pull = LL_GPIO_PULL_NO, 
                                      .clock = LL_AHB1_GRP1_PERIPH_GPIOB, .alternate = LL_GPIO_AF_7},
                                        
    [eGPIODriver_ModemUartCtsPin]  = {.port = GPIOD,                      .pin = LL_GPIO_PIN_4, 
                                      .mode = LL_GPIO_MODE_INPUT,         .speed = LL_GPIO_SPEED_FREQ_LOW,
                                      .output = LL_GPIO_OUTPUT_PUSHPULL,  .pull = LL_GPIO_PULL_NO, 
                                      .clock = LL_AHB1_GRP1_PERIPH_GPIOD, .alternate = LL_GPIO_AF_0},
                                            
    [eGPIODriver_DebugRxPin]       = {.port = GPIOB,                      .pin = LL_GPIO_PIN_7, 
                                      .mode = LL_GPIO_MODE_ALTERNATE,     .speed = LL_GPIO_SPEED_FREQ_VERY_HIGH,
                                      .output = LL_GPIO_OUTPUT_PUSHPULL,  .pull = LL_GPIO_PULL_NO, 
                                      .clock = LL_AHB1_GRP1_PERIPH_GPIOB, .alternate = LL_GPIO_AF_7},
                                        
    [eGPIODriver_ModemUartRtsPin]  = {.port = GPIOD,                      .pin = LL_GPIO_PIN_3, 
                                      .mode = LL_GPIO_MODE_OUTPUT,        .speed = LL_GPIO_SPEED_FREQ_LOW,
                                      .output = LL_GPIO_OUTPUT_PUSHPULL,  .pull = LL_GPIO_PULL_NO, 
                                      .clock = LL_AHB1_GRP1_PERIPH_GPIOD, .alternate = LL_GPIO_AF_0},
                                            
    [eGPIODriver_GnssOnPin]        = {.port = GPIOG,                      .pin = LL_GPIO_PIN_7, 
                                      .mode = LL_GPIO_MODE_OUTPUT,        .speed = LL_GPIO_SPEED_FREQ_LOW,
                                      .output = LL_GPIO_OUTPUT_PUSHPULL,  .pull = LL_GPIO_PULL_NO, 
                                      .clock = LL_AHB1_GRP1_PERIPH_GPIOG, .alternate = LL_GPIO_AF_0},

    [eGPIODriver_Reset_NPin]       = {.port = GPIOE,                      .pin = LL_GPIO_PIN_0,
                                      .mode = LL_GPIO_MODE_OUTPUT,        .speed = LL_GPIO_SPEED_FREQ_LOW,
                                      .output = LL_GPIO_OUTPUT_PUSHPULL,  .pull = LL_GPIO_PULL_NO, 
                                      .clock = LL_AHB1_GRP1_PERIPH_GPIOE, .alternate = LL_GPIO_AF_0},

    [eGPIODriver_ModemStatusPin]   = {.port = GPIOF,                      .pin = LL_GPIO_PIN_14,
                                      .mode = LL_GPIO_MODE_INPUT,         .speed = LL_GPIO_SPEED_FREQ_LOW,
                                      .output = LL_GPIO_OUTPUT_PUSHPULL,  .pull = LL_GPIO_PULL_NO, 
                                      .clock = LL_AHB1_GRP1_PERIPH_GPIOF, .alternate = LL_GPIO_AF_0}                                                                 
};
/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/
 
/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/
bool GPIO_Driver_Init (void) {
    LL_GPIO_InitTypeDef gpio_init_struct = {0};

    bool is_all_pins_initialized = true;

    for (eGPIODriver_t i = eGPIODriver_First; i < eGPIODriver_Last; i++) {
        LL_AHB1_GRP1_EnableClock(g_static_gpio_lut[i].clock);
        LL_GPIO_ResetOutputPin(g_static_gpio_lut[i].port, g_static_gpio_lut[i].pin);

        gpio_init_struct.Mode = g_static_gpio_lut[i].mode;
        gpio_init_struct.Speed = g_static_gpio_lut[i].speed;
        gpio_init_struct.OutputType = g_static_gpio_lut[i].output;
        gpio_init_struct.Pull = g_static_gpio_lut[i].pull;
        gpio_init_struct.Alternate = g_static_gpio_lut[i].alternate;
        gpio_init_struct.Pin = g_static_gpio_lut[i].pin;

        if (LL_GPIO_Init(g_static_gpio_lut[i].port, &gpio_init_struct) == ERROR) {
            is_all_pins_initialized = false;
        }
    }

    return is_all_pins_initialized;
}

bool GPIO_Driver_Toggle (eGPIODriver_t pin_name) {
    if (pin_name >= eGPIODriver_Last) {
        return false;
    }

    if (g_static_gpio_lut[pin_name].mode == LL_GPIO_MODE_OUTPUT) {
        LL_GPIO_TogglePin(g_static_gpio_lut[pin_name].port, g_static_gpio_lut[pin_name].pin);
        return true;
    }

    return false;
}

bool GPIO_Driver_Write (eGPIODriver_t pin_name, eGPIO_PinState_t pin_state) {
    if (pin_name >= eGPIODriver_Last) {
        return false;
    }

    if (g_static_gpio_lut[pin_name].mode != LL_GPIO_MODE_OUTPUT) {
        return false;
    }

    switch (pin_state) {
        case eGPIO_PinState_Low: {
            LL_GPIO_ResetOutputPin(g_static_gpio_lut[pin_name].port, g_static_gpio_lut[pin_name].pin);
        } break;
        case eGPIO_PinState_High: {
            LL_GPIO_SetOutputPin(g_static_gpio_lut[pin_name].port, g_static_gpio_lut[pin_name].pin);
        } break;
        default: {
            return false;
        }            
    }

    return true;
}

bool GPIO_Driver_Read (eGPIODriver_t pin_name, eGPIO_PinState_t *pin_state) {
    if (pin_name >= eGPIODriver_Last) {
        return false;
    }

    switch (g_static_gpio_lut[pin_name].mode) {
        case LL_GPIO_MODE_OUTPUT: {
            *pin_state = (LL_GPIO_IsOutputPinSet(g_static_gpio_lut[pin_name].port, g_static_gpio_lut[pin_name].pin) == 1) ? eGPIO_PinState_High : eGPIO_PinState_Low;
            return true;
        }
        case LL_GPIO_MODE_INPUT: {
            *pin_state = (LL_GPIO_IsInputPinSet(g_static_gpio_lut[pin_name].port, g_static_gpio_lut[pin_name].pin) == 1) ? eGPIO_PinState_High : eGPIO_PinState_Low;
            return true;
        }
        default: {
            break; 
        }
    }

    return false;
}
