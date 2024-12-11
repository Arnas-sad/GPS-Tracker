#ifndef SOURCE_DRIVER_GPIO_DRIVER_H_
#define SOURCE_DRIVER_GPIO_DRIVER_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdbool.h>
/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/
typedef enum {
    eGPIODriver_First = 0,
    eGPIODriver_ModemPowerOffPin = eGPIODriver_First,
    eGPIODriver_LedGpsFixPin, 
    eGPIODriver_ModemUartRXPin,
    eGPIODriver_ModemUartDtrPin,
    eGPIODriver_ModemOnPin,
    eGPIODriver_StatusLedPin,
    eGPIODriver_ModemUartTxPin,
    eGPIODriver_DebugTxPin,
    eGPIODriver_ModemUartCtsPin, 
    eGPIODriver_DebugRxPin,
    eGPIODriver_ModemUartRtsPin,
    eGPIODriver_GnssOnPin,
    eGPIODriver_Reset_NPin,
    eGPIODriver_ModemStatusPin,
    eGPIODriver_Last
} eGPIODriver_t;

typedef enum {
    eGPIO_PinState_First = 0,
    eGPIO_PinState_Low = eGPIO_PinState_First,
    eGPIO_PinState_High,
    eGPIO_PinState_Last
} eGPIO_PinState_t;
/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/
 bool GPIO_Driver_Init (void);
 bool GPIO_Driver_Toggle (eGPIODriver_t pin_name);
 bool GPIO_Driver_Write (eGPIODriver_t pin_name, eGPIO_PinState_t pin_state);
 bool GPIO_Driver_Read (eGPIODriver_t pin_name, eGPIO_PinState_t *pin_state);
#endif /* SOURCE_DRIVER_GPIO_DRIVER_H_ */
